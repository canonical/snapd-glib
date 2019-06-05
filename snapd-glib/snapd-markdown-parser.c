/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <ctype.h>

#include "config.h"

#include "snapd-markdown-parser.h"
#include "snapd-markdown-node.h"

/**
 * SECTION:snapd-markdown-parser
 * @short_description: Snap markdown text parser
 * @include: snapd-glib/snapd-glib.h
 *
 * A #SnapdMarkdownParser parses text formatted in markdown; for example the text returned by snapd_snap_get_description ().
 *
 * Snap supports the following subset of CommonMark (https://commonmark.org):
 * - Indented Code Blocks
 * - Paragraphs
 * - Blank Lines
 * - Unordered Lists
 * - Backslash escapes
 * - Code spans
 *
 * In addition text that contains URLs are converted into links.
 *
 * Use snapd_markdown_parser_parse () to convert text into a tree of
 * #SnapdMarkdownNode that you can then process to display in your client.
 */

/**
 * SnapdMarkdownParser:
 *
 * #SnapdMarkdownParser is an opaque data structure and can only be accessed
 * using the provided functions.
 *
 * Since: 1.48
 */
struct _SnapdMarkdownParser
{
    GObject parent_instance;
};

G_DEFINE_TYPE (SnapdMarkdownParser, snapd_markdown_parser, G_TYPE_OBJECT)

static gboolean
parse_empty_line (const gchar *line)
{
    for (int i = 0; line[i] != '\0'; i++)
        if (!isspace (line[i]))
            return FALSE;
    return TRUE;
}

static gboolean
parse_paragraph (const gchar *line, gchar **text)
{
    int i = 0;

    while (isspace (line[i]))
        i++;

    if (text != NULL)
        *text = g_strdup (line + i);

    return TRUE;
}

static gboolean
parse_bullet_list_item (const gchar *line, int *offset, gchar *symbol, gchar **bullet_text)
{
    int i = 0;

    while (isspace (line[i]))
        i++;
    gchar symbol_ = line[i];
    if (symbol_ != '-' && symbol_ != '+' && symbol_ != '*')
        return FALSE;
    int marker_offset = i;
    i++;
    if (line[i] == '\0')
        return FALSE;

    if (!isspace (line[i]))
        return FALSE;
    i++;

    int offset_ = i;
    while (isspace (line[offset_]))
        offset_++;

    /* Blank lines start one place after marker */
    if (line[offset_] == '\0')
       offset_ = marker_offset + 1;

    if (offset != NULL)
        *offset = offset_;
    if (symbol != NULL)
        *symbol = symbol_;
    if (bullet_text != NULL)
        *bullet_text = g_strdup (line + i);

    return TRUE;
}

static gboolean
parse_list_item_line (const gchar *line, int offset, gchar **text)
{
    for (int i = 0; i < offset; i++) {
        if (!isspace (line[i]))
            return FALSE;
    }

    if (text != NULL)
        *text = g_strdup (line + offset);

    return TRUE;
}

static gboolean
parse_indented_code_block (const gchar *line, gchar **text)
{
    int space_count = 0;

    while (line[space_count] == ' ')
        space_count++;
    if (space_count < 4)
        return FALSE;

    if (text != NULL)
        *text = g_strdup (line + 4);

    return TRUE;
}

static gboolean
is_punctuation_character (gchar c)
{
    return strchr ("!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~", c) != NULL;
    // FIXME: Also support unicode categories Pc, Pd, Pe, Pf, Pi, Po, and Ps.
}

static gboolean
is_left_flanking_delimiter_run (const gchar *text, int index)
{
    if (text[index] != '*' && text[index] != '_')
        return FALSE;

    int run_length = 1;
    while (text[index + run_length] == text[index])
        run_length++;

    /* 1) Must not be followed by whitespace */
    if (text[index + run_length] == '\0')
        return FALSE;
    if (isspace (text[index + run_length]))
        return FALSE;

    /* 2a) Is not followed by punctuation */
    if (!is_punctuation_character (text[index + run_length]))
        return TRUE;

    /* 2b) Followed by punctuation and preceeded by whitespace or punctuation */
    if (index == 0 || isspace (text[index - 1]))
        return TRUE;
    if (is_punctuation_character (text[index - 1]))
        return TRUE;

    return FALSE;
}

static gboolean
is_right_flanking_delimiter_run (const gchar *text, int index)
{
    if (text[index] != '*' && text[index] != '_')
        return FALSE;

    int run_length = 1;
    while (text[index + run_length] == text[index])
        run_length++;

    /* 1) Not preceeded by whitespace */
    if (index == 0 || isspace (text[index - 1]))
        return FALSE;

    /* 2a) No preceeded by punctuation */
    if (!is_punctuation_character (text[index - 1]))
        return TRUE;

    /* 2b) Preceeded by punctuation and followed by whitespace or punctuation */
    if (text[index + run_length] == '\0' || isspace (text[index + run_length]))
        return TRUE;
    if (is_punctuation_character (text[index + run_length]))
        return TRUE;

    return FALSE;
}

static gchar *
strip_text (const gchar *text)
{
    g_autoptr(GString) stripped_text = g_string_new ("");

    /* Strip leading whitespace */
    int i = 0;
    while (isspace (text[i]))
        i++;

    gboolean in_whitespace = FALSE;
    while (text[i] != '\0') {
        if (isspace (text[i]))
            in_whitespace = TRUE;
        else {
            if (in_whitespace)
                g_string_append_c (stripped_text, ' ');
            in_whitespace = FALSE;
            g_string_append_c (stripped_text, text[i]);
        }
        i++;
    }

    return g_steal_pointer (&stripped_text->str);
}

typedef struct
{
    gchar character;
    int length;
    gboolean can_open_emphasis;
    gboolean can_close_emphasis;
} EmphasisInfo;

static EmphasisInfo *
emphasis_info_new (void)
{
    EmphasisInfo *info;

    info = g_new0 (EmphasisInfo, 1);

    return info;
}

static void
emphasis_info_free (EmphasisInfo *info)
{
    g_free (info);
}

G_DEFINE_AUTOPTR_CLEANUP_FUNC (EmphasisInfo, emphasis_info_free)

SnapdMarkdownNode *
make_text_node (const gchar *text, int length)
{
    if (length > 0) {
        g_autofree gchar *subtext = g_strndup (text, length);
        return g_object_new (SNAPD_TYPE_MARKDOWN_NODE,
                             "node-type", SNAPD_MARKDOWN_NODE_TYPE_TEXT,
                             "text", subtext,
                             NULL);
    }
    else
        return g_object_new (SNAPD_TYPE_MARKDOWN_NODE,
                             "node-type", SNAPD_MARKDOWN_NODE_TYPE_TEXT,
                             "text", text,
                             NULL);
}

SnapdMarkdownNode *
make_paragraph_text_node (const gchar *text, int length)
{
    g_autoptr(GString) result = NULL;
    gchar last_c;

    result = g_string_new ("");
    last_c = '\0';
    for (int i = 0; text[i] != '\0' && (length == -1 || i < length); i++) {
        gchar c = text[i];
        if (isspace (c)) {
            if (!isspace (last_c))
                g_string_append_c (result, ' ');
        }
        else
            g_string_append_c (result, c);
        last_c = c;
    }

    return make_text_node (result->str, -1);
}

SnapdMarkdownNode *
make_delimiter_node (EmphasisInfo *info)
{
    g_autofree gchar *text = g_malloc (info->length + 1);
    for (int i = 0; i < info->length; i++)
        text[i] = info->character;
    text[info->length] = '\0';

    return g_object_new (SNAPD_TYPE_MARKDOWN_NODE,
                         "node-type", SNAPD_MARKDOWN_NODE_TYPE_TEXT,
                         "text", text,
                         NULL);
}

SnapdMarkdownNode *
make_code_node (SnapdMarkdownNodeType type, const gchar *text)
{
    g_autoptr(GPtrArray) children = NULL;

    children = g_ptr_array_new_with_free_func (g_object_unref);
    g_ptr_array_add (children, make_text_node (text, -1));
    return g_object_new (SNAPD_TYPE_MARKDOWN_NODE,
                         "node-type", type,
                         "children", children,
                         NULL);
}

static void
find_emphasis (GPtrArray *nodes, GHashTable *emphasis_info)
{
    int start_index, end_index;
    EmphasisInfo *start_info, *end_info;
    g_autoptr(GPtrArray) children = NULL;

    for (end_index = 0; end_index < nodes->len; end_index++) {
        SnapdMarkdownNode *start_node, *end_node;
        SnapdMarkdownNodeType node_type;

        end_node = g_ptr_array_index (nodes, end_index);
        end_info = g_hash_table_lookup (emphasis_info, end_node);
        if (end_info == NULL || !end_info->can_close_emphasis)
            continue;

        /* Find a start emphasis that matches this end */
        for (start_index = end_index - 1; start_index >= 0; start_index--) {
            start_node = g_ptr_array_index (nodes, start_index);
            start_info = g_hash_table_lookup (emphasis_info, start_node);
            if (start_info != NULL && start_info->can_open_emphasis &&
                start_info->character == end_info->character)
                break;
        }
        if (start_index < 0)
            continue;

        // FIXME: Can do if both a multiple of three
        if ((start_info->can_open_emphasis && start_info->can_close_emphasis) ||
            (end_info->can_open_emphasis && end_info->can_close_emphasis)) {
            if ((start_info->length + end_info->length) % 3 == 0)
                continue;
        }

        g_assert (start_info->length > 0);
        g_assert (end_info->length > 0);

        if (start_info->length > 1 && end_info->length > 1) {
            node_type = SNAPD_MARKDOWN_NODE_TYPE_STRONG_EMPHASIS;
            start_info->length -= 2;
            end_info->length -= 2;
        }
        else {
            node_type = SNAPD_MARKDOWN_NODE_TYPE_EMPHASIS;
            start_info->length--;
            end_info->length--;
        }

        /* Replace nodes */
        children = g_ptr_array_new_with_free_func (g_object_unref);
        for (int i = start_index + 1; i < end_index; i++) {
            SnapdMarkdownNode *node = g_ptr_array_index (nodes, i);
            g_ptr_array_add (children, g_object_ref (node));
        }
        g_ptr_array_remove_range (nodes, start_index, end_index - start_index + 1);
        g_ptr_array_insert (nodes, start_index,
                            g_object_new (SNAPD_TYPE_MARKDOWN_NODE,
                                          "node-type", node_type,
                                          "children", children,
                                          NULL));
        g_hash_table_steal (emphasis_info, start_node);
        g_hash_table_steal (emphasis_info, end_node);
        if (end_info->length > 0) {
            SnapdMarkdownNode *node;

            node = make_delimiter_node (end_info);
            g_hash_table_insert (emphasis_info, node, end_info);
            g_ptr_array_insert (nodes, start_index + 1, node);
        }
        else
            emphasis_info_free (end_info);
        if (start_info->length > 0) {
            SnapdMarkdownNode *node;

            node = make_delimiter_node (start_info);
            g_hash_table_insert (emphasis_info, node, start_info);
            g_ptr_array_insert (nodes, start_index, node);
        }
        else
            emphasis_info_free (start_info);
        end_index = start_index;
    }
}

static int
backtick_count (const gchar *text)
{
    int count = 0;
    while (text[count] == '`')
        count++;
    return count;
}

static void
combine_text_nodes (GPtrArray *nodes)
{
    for (int i = 0; i < nodes->len; i++) {
        SnapdMarkdownNode *node = g_ptr_array_index (nodes, i);
        GPtrArray *children;
        int node_count;
        g_autoptr(GString) text = NULL;

        children = snapd_markdown_node_get_children (node);
        if (children != NULL)
            combine_text_nodes (children);

        if (snapd_markdown_node_get_node_type (node) != SNAPD_MARKDOWN_NODE_TYPE_TEXT)
            continue;

        node_count = 1;
        while (i + node_count < nodes->len) {
            SnapdMarkdownNode *n = g_ptr_array_index (nodes, i + node_count);
            if (snapd_markdown_node_get_node_type (n) != SNAPD_MARKDOWN_NODE_TYPE_TEXT)
                break;
            if (text == NULL)
                text = g_string_new (snapd_markdown_node_get_text (node));
            g_string_append (text, snapd_markdown_node_get_text (n));
            node_count++;
        }

        if (node_count > 1) {
            g_ptr_array_remove_range (nodes, i, node_count);
            g_ptr_array_insert (nodes, i, make_text_node (text->str, -1));
        }
    }
}

static gboolean
is_valid_url_char (gchar c)
{
    if ((c & 0x80) != 0)
        return TRUE;

    if (c >= 'a' && c <= 'z')
        return TRUE;
    if (c >= 'A' && c <= 'Z')
        return TRUE;
    if (c >= '0' && c <= '9')
        return TRUE;
    /* "Safe" characters */
    if (strchr ("$-_.+", c) != NULL)
         return TRUE;
    /* "Reserved" characters */
    if (strchr (";/?:@&=", c) != NULL)
        return TRUE;
    if (strchr ("~#[]!'()*,%", c) != NULL)
        return TRUE;

    return FALSE;
}

static gboolean
is_url (const gchar *text, int *length)
{
    int prefix_length, _length, bracket_count;

    if (g_str_has_prefix (text, "http://"))
        prefix_length = 7;
    else if (g_str_has_prefix (text, "https://"))
        prefix_length = 8;
    else if (g_str_has_prefix (text, "mailto:"))
        prefix_length = 7;
    else
        return FALSE;

    _length = prefix_length;
    bracket_count = 0;
    while (text[_length] != '\0' && is_valid_url_char (text[_length])) {
        if (text[_length] == '(')
            bracket_count++;
        else if (text[_length] == ')') {
            bracket_count--;
            if (bracket_count < 0)
                break;
        }
        _length++;
    }
    if (_length == prefix_length)
        return FALSE;

    if (length != NULL)
        *length = _length;

    return TRUE;
}

static int
find_url (const gchar *text, int *url_length)
{
    for (int offset = 0; text[offset] != '\0'; offset++)
         if (is_url (text + offset, url_length))
             return offset;
    return -1;
}

SnapdMarkdownNode *
make_url_node (const gchar *text, int length)
{
    g_autoptr(GPtrArray) children = NULL;

    children = g_ptr_array_new_with_free_func (g_object_unref);
    g_ptr_array_add (children, make_text_node (text, length));
    return g_object_new (SNAPD_TYPE_MARKDOWN_NODE,
                         "node-type", SNAPD_MARKDOWN_NODE_TYPE_URL,
                         "children", children,
                         NULL);
}

static void
extract_urls (GPtrArray *nodes)
{
    for (int i = 0; i < nodes->len; i++) {
        SnapdMarkdownNode *node = g_ptr_array_index (nodes, i);
        GPtrArray *children;
        const gchar *text;
        int url_offset, url_length;

        children = snapd_markdown_node_get_children (node);
        if (snapd_markdown_node_get_node_type (node) != SNAPD_MARKDOWN_NODE_TYPE_URL && children != NULL)
            extract_urls (children);

        if (snapd_markdown_node_get_node_type (node) != SNAPD_MARKDOWN_NODE_TYPE_TEXT)
            continue;

        text = snapd_markdown_node_get_text (node);
        url_offset = find_url (text, &url_length);
        if (url_offset >= 0) {
            if (text[url_offset + url_length] != '\0')
                g_ptr_array_insert (nodes, i + 1, make_text_node (text + url_offset + url_length, -1));
            g_ptr_array_insert (nodes, i + 1, make_url_node (text + url_offset, url_length));
            if (url_offset > 0)
                g_ptr_array_insert (nodes, i + 1, make_text_node (text, url_offset));
            g_ptr_array_remove_index (nodes, i);
        }
    }
}

static GPtrArray *
markup_inline (SnapdMarkdownParser *parser, const gchar *text)
{
    g_autoptr(GPtrArray) nodes = NULL;
    g_autoptr(GHashTable) emphasis_info = NULL;

    /* Split into nodes */
    nodes = g_ptr_array_new_with_free_func (g_object_unref);
    emphasis_info = g_hash_table_new_full (g_direct_hash, g_direct_equal, NULL, (GDestroyNotify) emphasis_info_free);
    for (int i = 0; text[i] != '\0';) {
        int start = i;
        g_autoptr(EmphasisInfo) info = NULL;

        /* Extract code spans */
        if (text[start] == '`') {
            int size, end;

            size = backtick_count (text + start);
            end = start + size;
            while (text[end] != '\0') {
                 int s = backtick_count (text + end);
                 if (s == size)
                     break;
                 else if (s == 0)
                     end++;
                 else
                     end += s;
            }
            if (text[end] != '\0') {
                 g_autofree gchar *code_text = NULL;
                 g_autofree gchar *stripped_code_text = NULL;

                 code_text = g_strndup (text + start + size, end - start - size);
                 stripped_code_text = strip_text (code_text);
                 g_ptr_array_add (nodes, make_code_node (SNAPD_MARKDOWN_NODE_TYPE_CODE_SPAN, stripped_code_text));
                 i = end + size;
            }
            else
            {
                 g_ptr_array_add (nodes, make_paragraph_text_node (text + start, size));
                 i = start + size;
            }

            continue;
        }

        /* Escaped characters */
        if (text[start] == '\\' && is_punctuation_character (text[start + 1])) {
             g_ptr_array_add (nodes, make_text_node (text + start + 1, 1));
             i = start + 2;
             continue;
        }

        /* Extract emphasis delimiters (we determine later if they are used) */
        if (text[start] == '*' || text[start] == '_') {
            gboolean is_left_flanking, is_right_flanking;
            SnapdMarkdownNode *node;

            while (text[i] == text[start])
                i++;

            is_left_flanking = is_left_flanking_delimiter_run (text, start);
            is_right_flanking = is_right_flanking_delimiter_run (text, start);

            info = emphasis_info_new ();
            info->character = text[start];
            info->length = i - start;
            if (text[start] == '_') {
                info->can_open_emphasis = is_left_flanking && (!is_right_flanking || (start > 0 && is_punctuation_character (text[start - 1])));
                info->can_close_emphasis = is_right_flanking && (!is_left_flanking || is_punctuation_character (text[i]));
            }
            else if (text[start] == '*') {
                info->can_open_emphasis = is_left_flanking;
                info->can_close_emphasis = is_right_flanking;
            }
            node = make_paragraph_text_node (text + start, i - start);
            g_ptr_array_add (nodes, node);
            g_hash_table_insert (emphasis_info, node, g_steal_pointer (&info));
            continue;
        }

        /* Extract text until next potential emphasis or code span */
        while (text[i] != '\0') {
            if (text[i] == '*' || text[i] == '_' || text[i] == '`')
                break;

            if (text[i] == '\\' && is_punctuation_character (text[i + 1]))
                break;

            i++;
        }
        g_ptr_array_add (nodes, make_paragraph_text_node (text + start, i - start));
    }

    /* Convert nodes into emphasis */
    find_emphasis (nodes, emphasis_info);

    /* Combine sequential text nodes */
    combine_text_nodes (nodes);

    /* Extract URLs */
    extract_urls (nodes);

    return g_steal_pointer (&nodes);
}

static GPtrArray *
markdown_to_markup (SnapdMarkdownParser *parser, const gchar *text)
{
    g_autoptr(GPtrArray) nodes = g_ptr_array_new_with_free_func (g_object_unref);

    /* Snap supports the following subset of CommonMark (https://commonmark.org/):
     *
     * Indented code blocks
     * Paragraphs
     * Blank lines
     * Lists
     * Backslash escapes
     * Code spans
     * Emphasis and strong emphasis
     * Textual content
     *
     * In addition, links are automatically converted to hyperlinks.
     */

    /* 1. Split into lines */
    g_autoptr(GPtrArray) line_array = g_ptr_array_new ();
    int line_start = 0;
    for (int i = 0; text[i] != '\0'; i++) {
        if (text[i] == '\n' || text[i] == '\r') {
            if (text[i] == '\r' && text[i + 1] == '\n')
                i++;
            g_ptr_array_add (line_array, g_strndup (text + line_start, i - line_start + 1));
            line_start = i + 1;
        }
    }
    if (text[line_start] != '\0')
        g_ptr_array_add (line_array, g_strdup (text + line_start));
    g_ptr_array_add (line_array, NULL);
    GStrv lines = (GStrv) line_array->pdata;

    /* 2. Split lines into blocks (paragraphs, lists, code) */
    int line_number = 0;
    while (lines[line_number] != NULL) {
        /* Skip empty lines */
        if (parse_empty_line (lines[line_number])) {
            line_number++;
            continue;
        }

        g_autofree gchar *block_text = NULL;
        int bullet_offset = 0;
        gchar bullet_symbol;
        g_autofree gchar *bullet_text = NULL;
        /* Indented code blocks */
        if (parse_indented_code_block (lines[line_number], &block_text)) {
            g_autoptr(GString) code_text = g_string_new ("");
            g_string_append (code_text, block_text);

            while (TRUE) {
                line_number++;

                if (lines[line_number] == NULL)
                    break;

                g_autofree gchar *text = NULL;
                if (parse_indented_code_block (lines[line_number], &text)) {
                    g_string_append (code_text, text);
                }
                else if (parse_empty_line (lines[line_number]))
                    g_string_append_c (code_text, '\n');
                else
                    break;
            }

            /* Remove trailing empty lines */
            while (g_str_has_suffix (code_text->str, "\n\n"))
                g_string_truncate (code_text, code_text->len - 1);

            g_ptr_array_add (nodes, make_code_node (SNAPD_MARKDOWN_NODE_TYPE_CODE_BLOCK, code_text->str));
        }
        /* Bullet lists */
        else if (parse_bullet_list_item (lines[line_number], &bullet_offset, &bullet_symbol, &bullet_text)) {
            g_autoptr(GPtrArray) list_items = g_ptr_array_new_with_free_func (g_object_unref);
            g_autoptr(GString) list_data = g_string_new (bullet_text);
            gboolean starts_with_empty_line = bullet_text[0] == '\0';
            gboolean have_item = TRUE;
            while (TRUE) {
                line_number++;
                if (lines[line_number] == NULL)
                    break;

                if (parse_empty_line (lines[line_number])) {
                    if (starts_with_empty_line)
                        break;
                    g_string_append (list_data, lines[line_number]);
                    have_item = TRUE;
                    continue;
                }
                starts_with_empty_line = FALSE;
                g_autofree gchar *line_text = NULL;
                if (parse_list_item_line (lines[line_number], bullet_offset, &line_text)) {
                    g_string_append (list_data, line_text);
                    have_item = TRUE;
                    continue;
                }

                if (have_item) {
                    g_autoptr(GPtrArray) children = markdown_to_markup (parser, list_data->str);
                    g_ptr_array_add (list_items, g_object_new (SNAPD_TYPE_MARKDOWN_NODE,
                                                               "node-type", SNAPD_MARKDOWN_NODE_TYPE_LIST_ITEM,
                                                               "children", children,
                                                               NULL));
                    g_string_assign (list_data, "");
                    have_item = FALSE;
                }

                // FIXME: Check matching offset
                g_autofree gchar *text = NULL;
                gchar symbol;
                if (!parse_bullet_list_item (lines[line_number], &bullet_offset, &symbol, &text))
                    break;
                if (symbol != bullet_symbol)
                    break;
                g_string_assign (list_data, text);
                have_item = TRUE;
            }

            if (have_item) {
                g_autoptr(GPtrArray) children = markdown_to_markup (parser, list_data->str);
                g_ptr_array_add (list_items, g_object_new (SNAPD_TYPE_MARKDOWN_NODE,
                                                           "node-type", SNAPD_MARKDOWN_NODE_TYPE_LIST_ITEM,
                                                           "children", children,
                                                           NULL));
            }

            g_ptr_array_add (nodes, g_object_new (SNAPD_TYPE_MARKDOWN_NODE,
                                                  "node-type", SNAPD_MARKDOWN_NODE_TYPE_UNORDERED_LIST,
                                                  "children", list_items,
                                                  NULL));
        }
        /* Paragraphs */
        else {
            g_autoptr(GString) paragraph_text = g_string_new ("");
            while (TRUE) {
                g_autofree gchar *text = NULL;
                parse_paragraph (lines[line_number], &text);

                g_string_append (paragraph_text, text);

                line_number++;

                /* Out of data */
                if (lines[line_number] == NULL)
                    break;

                /* Break on empty line */
                if (parse_empty_line (lines[line_number]))
                    break;

                /* Break on non-empty list items */
                g_autofree gchar *bullet_text = NULL;
                if (parse_bullet_list_item (lines[line_number], NULL, NULL, &bullet_text)) {
                    if (bullet_text[0] != '\0')
                        break;
                }
            }

            /* Strip leading and trailing whitespace */
            int offset = 0;
            while (isspace (paragraph_text->str[offset]))
                offset++;
            int length = paragraph_text->len;
            while (length > 0 && isspace (paragraph_text->str[length - 1]))
                length--;
            if (offset + length > paragraph_text->len)
                length = 0;
            g_autofree gchar *stripped_text = g_strndup (paragraph_text->str + offset, length);

            g_autoptr(GPtrArray) children = markup_inline (parser, stripped_text);
            g_ptr_array_add (nodes, g_object_new (SNAPD_TYPE_MARKDOWN_NODE,
                                                  "node-type", SNAPD_MARKDOWN_NODE_TYPE_PARAGRAPH,
                                                  "children", children,
                                                  NULL));
        }
    }

    return g_steal_pointer (&nodes);
}

/**
 * snapd_markdown_parser_new:
 * @version: version supported by the client.
 *
 * Create an object to parse markdown text.
 *
 * Returns: a new #SnapdMarkdownParser
 *
 * Since: 1.48
 */
SnapdMarkdownParser *
snapd_markdown_parser_new (SnapdMarkdownVersion version)
{
    return g_object_new (SNAPD_TYPE_MARKDOWN_PARSER, NULL);
}

/**
 * snapd_markdown_parser_parse:
 * @parser: a #SnapdMarkdownParser.
 * @text: text to parse.
 *
 * Convert text in snapd markdown format to markup.
 *
 * Returns: (transfer container) (element-type SnapdMarkdownNode): Text split into blocks.
 *
 * Since: 1.48
 */
GPtrArray *
snapd_markdown_parser_parse (SnapdMarkdownParser *parser, const gchar *text)
{
    g_return_val_if_fail (SNAPD_IS_MARKDOWN_PARSER (parser), NULL);
    g_return_val_if_fail (text != NULL, NULL);
    return markdown_to_markup (parser, text);
}

static void
snapd_markdown_parser_class_init (SnapdMarkdownParserClass *klass)
{
}

static void
snapd_markdown_parser_init (SnapdMarkdownParser *parser)
{
}
