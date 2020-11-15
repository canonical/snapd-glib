/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <string.h>
#include <snapd-glib/snapd-glib.h>

static gchar *
escape_text (const gchar *text)
{
    g_autoptr(GString) escaped_text = g_string_new ("");

    for (const gchar *c = text; *c != '\0'; c++) {
        if (*c == '&')
            g_string_append (escaped_text, "&amp;");
        else if (*c == '<')
            g_string_append (escaped_text, "&lt;");
        else if (*c == '>')
            g_string_append (escaped_text, "&gt;");
        else if (*c == '"')
            g_string_append (escaped_text, "&quot;");
        else
            g_string_append_c (escaped_text, *c);
    }

    return g_steal_pointer (&escaped_text->str);
}

static gchar *serialize_node (SnapdMarkdownNode *node);

static gchar *
serialize_nodes (GPtrArray *nodes)
{
    g_autoptr(GString) text = g_string_new ("");

    for (int i = 0; i < nodes->len; i++) {
        SnapdMarkdownNode *node = g_ptr_array_index (nodes, i);
        g_autofree gchar *node_text = serialize_node (node);
        g_string_append (text, node_text);
    }

    return g_steal_pointer (&text->str);
}

static gchar *
serialize_node (SnapdMarkdownNode *node)
{
   GPtrArray *children = snapd_markdown_node_get_children (node);

   g_autofree gchar *contents = NULL;
   switch (snapd_markdown_node_get_node_type (node)) {
   case SNAPD_MARKDOWN_NODE_TYPE_TEXT:
       return escape_text (snapd_markdown_node_get_text (node));

   case SNAPD_MARKDOWN_NODE_TYPE_PARAGRAPH:
       contents = serialize_nodes (children);
       return g_strdup_printf ("<p>%s</p>\n", contents);

   case SNAPD_MARKDOWN_NODE_TYPE_UNORDERED_LIST:
       contents = serialize_nodes (children);
       return g_strdup_printf ("<ul>\n%s</ul>\n", contents);

   case SNAPD_MARKDOWN_NODE_TYPE_LIST_ITEM:
       if (children->len == 0)
           return g_strdup ("<li></li>\n");
       if (children->len == 1) {
           SnapdMarkdownNode *child = g_ptr_array_index (children, 0);
           if (snapd_markdown_node_get_node_type (child) == SNAPD_MARKDOWN_NODE_TYPE_PARAGRAPH) {
               contents = serialize_nodes (snapd_markdown_node_get_children (child));
               return g_strdup_printf ("<li>%s</li>\n", contents);
           }
       }
       contents = serialize_nodes (children);
       return g_strdup_printf ("<li>\n%s</li>\n", contents);

   case SNAPD_MARKDOWN_NODE_TYPE_CODE_BLOCK:
       contents = serialize_nodes (children);
       return g_strdup_printf ("<pre><code>%s</code></pre>\n", contents);

   case SNAPD_MARKDOWN_NODE_TYPE_CODE_SPAN:
       contents = serialize_nodes (children);
       return g_strdup_printf ("<code>%s</code>", contents);

   case SNAPD_MARKDOWN_NODE_TYPE_EMPHASIS:
       contents = serialize_nodes (children);
       return g_strdup_printf ("<em>%s</em>", contents);

   case SNAPD_MARKDOWN_NODE_TYPE_STRONG_EMPHASIS:
       contents = serialize_nodes (children);
       return g_strdup_printf ("<strong>%s</strong>", contents);

   case SNAPD_MARKDOWN_NODE_TYPE_URL:
       return serialize_nodes (children);

   default:
       g_assert_not_reached ();
       return g_strdup ("");
   }
}

static gchar *
parse (const gchar *text)
{
    g_autoptr(SnapdMarkdownParser) parser = snapd_markdown_parser_new (SNAPD_MARKDOWN_VERSION_0);
    snapd_markdown_parser_set_preserve_whitespace (parser, TRUE);
    g_autoptr(GPtrArray) nodes = snapd_markdown_parser_parse (parser, text);
    return serialize_nodes (nodes);
}

static void
test_markdown_empty (void)
{
    g_autofree gchar *markup = parse ("");
    g_assert_cmpstr (markup, ==, "");
}

static void
test_markdown_single_character (void)
{
    g_autofree gchar *markup = parse ("a");
    g_assert_cmpstr (markup, ==, "<p>a</p>\n");
}

/* The following tests are a subset of those in the CommonMark spec (https://spec.commonmark.org/0.28).
 * Some tests are modified to match the expected snap behaviour */

static void
test_markdown_precedence (void)
{
    g_autofree gchar *example12 = parse ("- `one\n- two`\n");
    g_assert_cmpstr (example12, ==, "<ul>\n<li>`one</li>\n<li>two`</li>\n</ul>\n");
}

static void
test_markdown_indented_code (void)
{
    g_autofree gchar *example76 = parse ("    a simple\n      indented code block\n");
    g_assert_cmpstr (example76, ==, "<pre><code>a simple\n  indented code block\n</code></pre>\n");

    g_autofree gchar *example77 = parse ("  - foo\n\n    bar\n");
    g_assert_cmpstr (example77, ==, "<ul>\n<li>\n<p>foo</p>\n<p>bar</p>\n</li>\n</ul>\n");

    g_autofree gchar *example79 = parse ("    <a/>\n    *hi*\n\n    - one\n");
    g_assert_cmpstr (example79, ==, "<pre><code>&lt;a/&gt;\n*hi*\n\n- one\n</code></pre>\n");

    g_autofree gchar *example80 = parse ("    chunk1\n\n    chunk2\n  \n \n \n    chunk3\n");
    g_assert_cmpstr (example80, ==, "<pre><code>chunk1\n\nchunk2\n\n\n\nchunk3\n</code></pre>\n");

    g_autofree gchar *example81 = parse ("    chunk1\n      \n      chunk2\n");
    g_assert_cmpstr (example81, ==, "<pre><code>chunk1\n  \n  chunk2\n</code></pre>\n");

    g_autofree gchar *example82 = parse ("Foo\n    bar\n\n");
    g_assert_cmpstr (example82, ==, "<p>Foo\nbar</p>\n");

    g_autofree gchar *example83 = parse ("    foo\nbar\n");
    g_assert_cmpstr (example83, ==, "<pre><code>foo\n</code></pre>\n<p>bar</p>\n");

    g_autofree gchar *example85 = parse ("        foo\n    bar\n");
    g_assert_cmpstr (example85, ==, "<pre><code>    foo\nbar\n</code></pre>\n");

    g_autofree gchar *example86 = parse ("\n    \n    foo\n    \n\n");
    g_assert_cmpstr (example86, ==, "<pre><code>foo\n</code></pre>\n");

    g_autofree gchar *example87 = parse ("    foo  \n");
    g_assert_cmpstr (example87, ==, "<pre><code>foo  \n</code></pre>\n");
}

static void
test_markdown_paragraphs (void)
{
    g_autofree gchar *example182 = parse ("aaa\n\nbbb\n");
    g_assert_cmpstr (example182, ==, "<p>aaa</p>\n<p>bbb</p>\n");

    g_autofree gchar *example183 = parse ("aaa\nbbb\n\nccc\nddd\n");
    g_assert_cmpstr (example183, ==, "<p>aaa\nbbb</p>\n<p>ccc\nddd</p>\n");

    g_autofree gchar *example184 = parse ("aaa\n\n\nbbb\n");
    g_assert_cmpstr (example184, ==, "<p>aaa</p>\n<p>bbb</p>\n");

    g_autofree gchar *example185 = parse ("  aaa\n bbb\n");
    g_assert_cmpstr (example185, ==, "<p>aaa\nbbb</p>\n");

    g_autofree gchar *example186 = parse ("aaa\n             bbb\n                                       ccc\n");
    g_assert_cmpstr (example186, ==, "<p>aaa\nbbb\nccc</p>\n");

    g_autofree gchar *example187 = parse ("   aaa\nbbb\n");
    g_assert_cmpstr (example187, ==, "<p>aaa\nbbb</p>\n");

    g_autofree gchar *example188 = parse ("    aaa\nbbb\n");
    g_assert_cmpstr (example188, ==, "<pre><code>aaa\n</code></pre>\n<p>bbb</p>\n");
}

static void
test_markdown_list_items (void)
{
    g_autofree gchar *example218 = parse ("- one\n\n two\n");
    g_assert_cmpstr (example218, ==, "<ul>\n<li>one</li>\n</ul>\n<p>two</p>\n");

    g_autofree gchar *example219 = parse ("- one\n\n  two\n");
    g_assert_cmpstr (example219, ==, "<ul>\n<li>\n<p>one</p>\n<p>two</p>\n</li>\n</ul>\n");

    g_autofree gchar *example220 = parse (" -    one\n\n     two\n");
    g_assert_cmpstr (example220, ==, "<ul>\n<li>one</li>\n</ul>\n<pre><code> two\n</code></pre>\n");

    g_autofree gchar *example221 = parse (" -    one\n\n      two\n");
    g_assert_cmpstr (example221, ==, "<ul>\n<li>\n<p>one</p>\n<p>two</p>\n</li>\n</ul>\n");

    g_autofree gchar *example224 = parse ("-one\n\n2.two\n");
    g_assert_cmpstr (example224, ==, "<p>-one</p>\n<p>2.two</p>\n");

    g_autofree gchar *example225 = parse ("- foo\n\n\n  bar\n");
    g_assert_cmpstr (example225, ==, "<ul>\n<li>\n<p>foo</p>\n<p>bar</p>\n</li>\n</ul>\n");

    g_autofree gchar *example227 = parse ("- Foo\n\n      bar\n\n\n      baz\n");
    g_assert_cmpstr (example227, ==, "<ul>\n<li>\n<p>Foo</p>\n<pre><code>bar\n\n\nbaz\n</code></pre>\n</li>\n</ul>\n");

    g_autofree gchar *example229 = parse ("1234567890. not ok\n");
    g_assert_cmpstr (example229, ==, "<p>1234567890. not ok</p>\n");

    g_autofree gchar *example232 = parse ("-1. not ok\n");
    g_assert_cmpstr (example232, ==, "<p>-1. not ok</p>\n");

    g_autofree gchar *example233 = parse ("- foo\n\n      bar\n");
    g_assert_cmpstr (example233, ==, "<ul>\n<li>\n<p>foo</p>\n<pre><code>bar\n</code></pre>\n</li>\n</ul>\n");

    g_autofree gchar *example235 = parse ("    indented code\n\nparagraph\n\n    more code\n");
    g_assert_cmpstr (example235, ==, "<pre><code>indented code\n</code></pre>\n<p>paragraph</p>\n<pre><code>more code\n</code></pre>\n");

    g_autofree gchar *example238 = parse ("   foo\n\nbar\n");
    g_assert_cmpstr (example238, ==, "<p>foo</p>\n<p>bar</p>\n");

    g_autofree gchar *example239 = parse ("-    foo\n\n  bar\n");
    g_assert_cmpstr (example239, ==, "<ul>\n<li>foo</li>\n</ul>\n<p>bar</p>\n");

    g_autofree gchar *example240 = parse ("-  foo\n\n   bar\n");
    g_assert_cmpstr (example240, ==, "<ul>\n<li>\n<p>foo</p>\n<p>bar</p>\n</li>\n</ul>\n");

    g_autofree gchar *example242 = parse ("-   \n  foo\n");
    g_assert_cmpstr (example242, ==, "<ul>\n<li>foo</li>\n</ul>\n");

    g_autofree gchar *example243 = parse ("-\n\n  foo\n");
    g_assert_cmpstr (example243, ==, "<ul>\n<li></li>\n</ul>\n<p>foo</p>\n");

    g_autofree gchar *example244 = parse ("- foo\n-\n- bar\n");
    g_assert_cmpstr (example244, ==, "<ul>\n<li>foo</li>\n<li></li>\n<li>bar</li>\n</ul>\n");

    g_autofree gchar *example245 = parse ("- foo\n-   \n- bar\n");
    g_assert_cmpstr (example245, ==, "<ul>\n<li>foo</li>\n<li></li>\n<li>bar</li>\n</ul>\n");

    g_autofree gchar *example247 = parse ("*\n");
    g_assert_cmpstr (example247, ==, "<ul>\n<li></li>\n</ul>\n");

    g_autofree gchar *example248 = parse ("foo\n*\n\nfoo\n1.\n");
    g_assert_cmpstr (example248, ==, "<p>foo\n*</p>\n<p>foo\n1.</p>\n");

    g_autofree gchar *example252 = parse ("    1.  A paragraph\n        with two lines.\n\n            indented code\n\n        > A block quote.\n");
    g_assert_cmpstr (example252, ==, "<pre><code>1.  A paragraph\n    with two lines.\n\n        indented code\n\n    &gt; A block quote.\n</code></pre>\n");

    g_autofree gchar *example257 = parse ("- foo\n  - bar\n    - baz\n      - boo\n");
    g_assert_cmpstr (example257, ==, "<ul>\n<li>\n<p>foo</p>\n<ul>\n<li>\n<p>bar</p>\n<ul>\n<li>\n<p>baz</p>\n<ul>\n<li>boo</li>\n</ul>\n</li>\n</ul>\n</li>\n</ul>\n</li>\n</ul>\n");

    g_autofree gchar *example258 = parse ("- foo\n - bar\n  - baz\n   - boo\n");
    g_assert_cmpstr (example258, ==, "<ul>\n<li>foo</li>\n<li>bar</li>\n<li>baz</li>\n<li>boo</li>\n</ul>\n");

    g_autofree gchar *example261 = parse ("- - foo\n");
    g_assert_cmpstr (example261, ==, "<ul>\n<li>\n<ul>\n<li>foo</li>\n</ul>\n</li>\n</ul>\n");
}

static void
test_markdown_lists (void)
{
    g_autofree gchar *example264 = parse ("- foo\n- bar\n+ baz\n");
    g_assert_cmpstr (example264, ==, "<ul>\n<li>foo</li>\n<li>bar</li>\n</ul>\n<ul>\n<li>baz</li>\n</ul>\n");

    g_autofree gchar *example266 = parse ("Foo\n- bar\n- baz\n");
    g_assert_cmpstr (example266, ==, "<p>Foo</p>\n<ul>\n<li>bar</li>\n<li>baz</li>\n</ul>\n");

    g_autofree gchar *example267 = parse ("The number of windows in my house is\n14.  The number of doors is 6.\n");
    g_assert_cmpstr (example267, ==, "<p>The number of windows in my house is\n14.  The number of doors is 6.</p>\n");

    g_autofree gchar *example269 = parse ("- foo\n\n- bar\n\n\n- baz\n");
    g_assert_cmpstr (example269, ==, "<ul>\n<li>foo</li>\n<li>bar</li>\n<li>baz</li>\n</ul>\n");

    g_autofree gchar *example270 = parse ("- foo\n  - bar\n    - baz\n\n\n      bim\n");
    g_assert_cmpstr (example270, ==, "<ul>\n<li>\n<p>foo</p>\n<ul>\n<li>\n<p>bar</p>\n<ul>\n<li>\n<p>baz</p>\n<p>bim</p>\n</li>\n</ul>\n</li>\n</ul>\n</li>\n</ul>\n");

    g_autofree gchar *example273 = parse ("- a\n - b\n  - c\n   - d\n    - e\n   - f\n  - g\n - h\n- i\n");
    g_assert_cmpstr (example273, ==, "<ul>\n<li>a</li>\n<li>b</li>\n<li>c</li>\n<li>d</li>\n<li>e</li>\n<li>f</li>\n<li>g</li>\n<li>h</li>\n<li>i</li>\n</ul>\n");

    g_autofree gchar *example275 = parse ("- a\n- b\n\n- c\n");
    g_assert_cmpstr (example275, ==, "<ul>\n<li>a</li>\n<li>b</li>\n<li>c</li>\n</ul>\n");

    g_autofree gchar *example276 = parse ("* a\n*\n\n* c\n");
    g_assert_cmpstr (example276, ==, "<ul>\n<li>a</li>\n<li></li>\n<li>c</li>\n</ul>\n");

    g_autofree gchar *example277 = parse ("- a\n- b\n\n  c\n- d\n");
    g_assert_cmpstr (example277, ==, "<ul>\n<li>a</li>\n<li>\n<p>b</p>\n<p>c</p>\n</li>\n<li>d</li>\n</ul>\n");

    g_autofree gchar *example280 = parse ("- a\n  - b\n\n    c\n- d\n");
    g_assert_cmpstr (example280, ==, "<ul>\n<li>\n<p>a</p>\n<ul>\n<li>\n<p>b</p>\n<p>c</p>\n</li>\n</ul>\n</li>\n<li>d</li>\n</ul>\n");

    g_autofree gchar *example283 = parse ("- a\n");
    g_assert_cmpstr (example283, ==, "<ul>\n<li>a</li>\n</ul>\n");

    g_autofree gchar *example284 = parse ("- a\n  - b\n");
    g_assert_cmpstr (example284, ==, "<ul>\n<li>\n<p>a</p>\n<ul>\n<li>b</li>\n</ul>\n</li>\n</ul>\n");

    g_autofree gchar *example286 = parse ("* foo\n  * bar\n\n  baz\n");
    g_assert_cmpstr (example286, ==, "<ul>\n<li>\n<p>foo</p>\n<ul>\n<li>bar</li>\n</ul>\n<p>baz</p>\n</li>\n</ul>\n");

    g_autofree gchar *example287 = parse ("- a\n  - b\n  - c\n\n- d\n  - e\n  - f\n");
    g_assert_cmpstr (example287, ==, "<ul>\n<li>\n<p>a</p>\n<ul>\n<li>b</li>\n<li>c</li>\n</ul>\n</li>\n<li>\n<p>d</p>\n<ul>\n<li>e</li>\n<li>f</li>\n</ul>\n</li>\n</ul>\n");
}

static void
test_markdown_inlines (void)
{
    g_autofree gchar *example288 = parse ("`hi`lo`\n");
    g_assert_cmpstr (example288, ==, "<p><code>hi</code>lo`</p>\n");

    g_autofree gchar *example289 = parse ("\\!\\\"\\#\\$\\%\\&\\'\\(\\)\\*\\+\\,\\-\\.\\/\\:\\;\\<\\=\\>\\?\\@\\[\\\\\\]\\^\\_\\`\\{\\|\\}\\~\n");
    g_assert_cmpstr (example289, ==, "<p>!&quot;#$%&amp;'()*+,-./:;&lt;=&gt;?@[\\]^_`{|}~</p>\n");

    g_autofree gchar *example290 = parse ("\\\t\\A\\a\\ \\3\\φ\\«\n");
    g_assert_cmpstr (example290, ==, "<p>\\\t\\A\\a\\ \\3\\φ\\«</p>\n");

    g_autofree gchar *example291 = parse ("\\*not emphasized*\n\\<br/> not a tag\n\\[not a link](/foo)\n\\`not code`\n1\\. not a list\n\\* not a list\n\\# not a heading\n\\[foo]: /url \"not a reference\"\n");
    g_assert_cmpstr (example291, ==, "<p>*not emphasized*\n&lt;br/&gt; not a tag\n[not a link](/foo)\n`not code`\n1. not a list\n* not a list\n# not a heading\n[foo]: /url &quot;not a reference&quot;</p>\n");

    g_autofree gchar *example292 = parse ("\\\\*emphasis*\n");
    g_assert_cmpstr (example292, ==, "<p>\\<em>emphasis</em></p>\n");

    g_autofree gchar *example295 = parse ("    \\[\\]\n");
    g_assert_cmpstr (example295, ==, "<pre><code>\\[\\]\n</code></pre>\n");
}

static void
test_markdown_code_spans (void)
{
    g_autofree gchar *example314 = parse ("`foo`\n");
    g_assert_cmpstr (example314, ==, "<p><code>foo</code></p>\n");

    g_autofree gchar *example315 = parse ("`` foo ` bar  ``\n");
    g_assert_cmpstr (example315, ==, "<p><code>foo ` bar</code></p>\n");

    g_autofree gchar *example316 = parse ("` `` `\n");
    g_assert_cmpstr (example316, ==, "<p><code>``</code></p>\n");

    g_autofree gchar *example317 = parse ("``\nfoo\n``\n");
    g_assert_cmpstr (example317, ==, "<p><code>foo</code></p>\n");

    g_autofree gchar *example318 = parse ("`foo   bar\n  baz`\n");
    g_assert_cmpstr (example318, ==, "<p><code>foo bar baz</code></p>\n");

    g_autofree gchar *example319 = parse ("`a  b`\n");
    g_assert_cmpstr (example319, ==, "<p><code>a  b</code></p>\n");

    g_autofree gchar *example320 = parse ("`foo `` bar`\n");
    g_assert_cmpstr (example320, ==, "<p><code>foo `` bar</code></p>\n");

    g_autofree gchar *example321 = parse ("`foo\\`bar`\n");
    g_assert_cmpstr (example321, ==, "<p><code>foo\\</code>bar`</p>\n");

    g_autofree gchar *example322 = parse ("*foo`*`\n");
    g_assert_cmpstr (example322, ==, "<p>*foo<code>*</code></p>\n");

    g_autofree gchar *example326 = parse ("`<http://foo.bar.`baz>`\n");
    g_assert_cmpstr (example326, ==, "<p><code>&lt;http://foo.bar.</code>baz&gt;`</p>\n");

    g_autofree gchar *example328 = parse ("```foo``\n");
    g_assert_cmpstr (example328, ==, "<p>```foo``</p>\n");

    g_autofree gchar *example329 = parse ("`foo\n");
    g_assert_cmpstr (example329, ==, "<p>`foo</p>\n");

    g_autofree gchar *example330 = parse ("`foo``bar``\n");
    g_assert_cmpstr (example330, ==, "<p>`foo<code>bar</code></p>\n");
}

static void
test_markdown_emphasis (void)
{
    g_autofree gchar *example331 = parse ("*foo bar*\n");
    g_assert_cmpstr (example331, ==, "<p><em>foo bar</em></p>\n");

    g_autofree gchar *example332 = parse ("a * foo bar*\n");
    g_assert_cmpstr (example332, ==, "<p>a * foo bar*</p>\n");

    g_autofree gchar *example333 = parse ("a*\"foo\"*\n");
    g_assert_cmpstr (example333, ==, "<p>a*&quot;foo&quot;*</p>\n");

    g_autofree gchar *example334 = parse ("* a *\n");
    //FIXME g_assert_cmpstr (example334, ==, "<p>* a *</p>\n");

    g_autofree gchar *example335 = parse ("foo*bar*\n");
    g_assert_cmpstr (example335, ==, "<p>foo<em>bar</em></p>\n");

    g_autofree gchar *example336 = parse ("5*6*78\n");
    g_assert_cmpstr (example336, ==, "<p>5<em>6</em>78</p>\n");

    g_autofree gchar *example337 = parse ("_foo bar_\n");
    g_assert_cmpstr (example337, ==, "<p><em>foo bar</em></p>\n");

    g_autofree gchar *example338 = parse ("_ foo bar_\n");
    g_assert_cmpstr (example338, ==, "<p>_ foo bar_</p>\n");

    g_autofree gchar *example339 = parse ("a_\"foo\"_\n");
    g_assert_cmpstr (example339, ==, "<p>a_&quot;foo&quot;_</p>\n");

    g_autofree gchar *example340 = parse ("foo_bar_\n");
    g_assert_cmpstr (example340, ==, "<p>foo_bar_</p>\n");

    g_autofree gchar *example341 = parse ("5_6_78\n");
    g_assert_cmpstr (example341, ==, "<p>5_6_78</p>\n");

    g_autofree gchar *example342 = parse ("пристаням_стремятся_\n");
    g_assert_cmpstr (example342, ==, "<p>пристаням_стремятся_</p>\n");

    g_autofree gchar *example343 = parse ("aa_\"bb\"_cc\n");
    g_assert_cmpstr (example343, ==, "<p>aa_&quot;bb&quot;_cc</p>\n");

    g_autofree gchar *example344 = parse ("foo-_(bar)_\n");
    g_assert_cmpstr (example344, ==, "<p>foo-<em>(bar)</em></p>\n");

    g_autofree gchar *example345 = parse ("_foo*\n");
    g_assert_cmpstr (example345, ==, "<p>_foo*</p>\n");

    g_autofree gchar *example346 = parse ("*foo bar *\n");
    g_assert_cmpstr (example346, ==, "<p>*foo bar *</p>\n");

    g_autofree gchar *example347 = parse ("*foo bar\n*\n");
    g_assert_cmpstr (example347, ==, "<p>*foo bar\n*</p>\n");

    g_autofree gchar *example348 = parse ("*(*foo)\n");
    g_assert_cmpstr (example348, ==, "<p>*(*foo)</p>\n");

    g_autofree gchar *example349 = parse ("*(*foo*)*\n");
    g_assert_cmpstr (example349, ==, "<p><em>(<em>foo</em>)</em></p>\n");

    g_autofree gchar *example350 = parse ("*foo*bar\n");
    g_assert_cmpstr (example350, ==, "<p><em>foo</em>bar</p>\n");

    g_autofree gchar *example351 = parse ("_foo bar _\n");
    g_assert_cmpstr (example351, ==, "<p>_foo bar _</p>\n");

    g_autofree gchar *example352 = parse ("_(_foo)\n");
    g_assert_cmpstr (example352, ==, "<p>_(_foo)</p>\n");

    g_autofree gchar *example353 = parse ("_(_foo_)_\n");
    g_assert_cmpstr (example353, ==, "<p><em>(<em>foo</em>)</em></p>\n");

    g_autofree gchar *example354 = parse ("_foo_bar\n");
    g_assert_cmpstr (example354, ==, "<p>_foo_bar</p>\n");

    g_autofree gchar *example355 = parse ("_пристаням_стремятся\n");
    g_assert_cmpstr (example355, ==, "<p>_пристаням_стремятся</p>\n");

    g_autofree gchar *example356 = parse ("_foo_bar_baz_\n");
    g_assert_cmpstr (example356, ==, "<p><em>foo_bar_baz</em></p>\n");

    g_autofree gchar *example357 = parse ("_(bar)_.\n");
    g_assert_cmpstr (example357, ==, "<p><em>(bar)</em>.</p>\n");

    g_autofree gchar *example358 = parse ("**foo bar**\n");
    g_assert_cmpstr (example358, ==, "<p><strong>foo bar</strong></p>\n");

    g_autofree gchar *example359 = parse ("** foo bar**\n");
    g_assert_cmpstr (example359, ==, "<p>** foo bar**</p>\n");

    g_autofree gchar *example360 = parse ("a**\"foo\"**\n");
    g_assert_cmpstr (example360, ==, "<p>a**&quot;foo&quot;**</p>\n");

    g_autofree gchar *example361 = parse ("foo**bar**\n");
    g_assert_cmpstr (example361, ==, "<p>foo<strong>bar</strong></p>\n");

    g_autofree gchar *example362 = parse ("__foo bar__\n");
    g_assert_cmpstr (example362, ==, "<p><strong>foo bar</strong></p>\n");

    g_autofree gchar *example363 = parse ("__ foo bar__\n");
    g_assert_cmpstr (example363, ==, "<p>__ foo bar__</p>\n");

    g_autofree gchar *example364 = parse ("__\nfoo bar__\n");
    g_assert_cmpstr (example364, ==, "<p>__\nfoo bar__</p>\n");

    g_autofree gchar *example365 = parse ("a__\"foo\"__\n");
    g_assert_cmpstr (example365, ==, "<p>a__&quot;foo&quot;__</p>\n");

    g_autofree gchar *example366 = parse ("foo__bar__\n");
    g_assert_cmpstr (example366, ==, "<p>foo__bar__</p>\n");

    g_autofree gchar *example367 = parse ("5__6__78\n");
    g_assert_cmpstr (example367, ==, "<p>5__6__78</p>\n");

    g_autofree gchar *example368 = parse ("пристаням__стремятся__\n");
    g_assert_cmpstr (example368, ==, "<p>пристаням__стремятся__</p>\n");

    g_autofree gchar *example369 = parse ("__foo, __bar__, baz__\n");
    g_assert_cmpstr (example369, ==, "<p><strong>foo, <strong>bar</strong>, baz</strong></p>\n");

    g_autofree gchar *example370 = parse ("foo-__(bar)__\n");
    g_assert_cmpstr (example370, ==, "<p>foo-<strong>(bar)</strong></p>\n");

    g_autofree gchar *example371 = parse ("**foo bar **\n");
    g_assert_cmpstr (example371, ==, "<p>**foo bar **</p>\n");

    g_autofree gchar *example372 = parse ("**(**foo)\n");
    g_assert_cmpstr (example372, ==, "<p>**(**foo)</p>\n");

    g_autofree gchar *example373 = parse ("*(**foo**)*\n");
    g_assert_cmpstr (example373, ==, "<p><em>(<strong>foo</strong>)</em></p>\n");

    g_autofree gchar *example374 = parse ("**Gomphocarpus (*Gomphocarpus physocarpus*, syn.\n*Asclepias physocarpa*)**\n");
    g_assert_cmpstr (example374, ==, "<p><strong>Gomphocarpus (<em>Gomphocarpus physocarpus</em>, syn.\n<em>Asclepias physocarpa</em>)</strong></p>\n");

    g_autofree gchar *example375 = parse ("**foo \"*bar*\" foo**\n");
    g_assert_cmpstr (example375, ==, "<p><strong>foo &quot;<em>bar</em>&quot; foo</strong></p>\n");

    g_autofree gchar *example376 = parse ("**foo**bar\n");
    g_assert_cmpstr (example376, ==, "<p><strong>foo</strong>bar</p>\n");

    g_autofree gchar *example377 = parse ("__foo bar __\n");
    g_assert_cmpstr (example377, ==, "<p>__foo bar __</p>\n");

    g_autofree gchar *example378 = parse ("__(__foo)\n");
    g_assert_cmpstr (example378, ==, "<p>__(__foo)</p>\n");

    g_autofree gchar *example379 = parse ("_(__foo__)_\n");
    g_assert_cmpstr (example379, ==, "<p><em>(<strong>foo</strong>)</em></p>\n");

    g_autofree gchar *example380 = parse ("__foo__bar\n");
    g_assert_cmpstr (example380, ==, "<p>__foo__bar</p>\n");

    g_autofree gchar *example381 = parse ("__пристаням__стремятся\n");
    g_assert_cmpstr (example381, ==, "<p>__пристаням__стремятся</p>\n");

    g_autofree gchar *example382 = parse ("__foo__bar__baz__\n");
    g_assert_cmpstr (example382, ==, "<p><strong>foo__bar__baz</strong></p>\n");

    g_autofree gchar *example383 = parse ("__(bar)__.\n");
    g_assert_cmpstr (example383, ==, "<p><strong>(bar)</strong>.</p>\n");

    g_autofree gchar *example385 = parse ("*foo\nbar*\n");
    g_assert_cmpstr (example385, ==, "<p><em>foo\nbar</em></p>\n");

    g_autofree gchar *example386 = parse ("_foo __bar__ baz_\n");
    g_assert_cmpstr (example386, ==, "<p><em>foo <strong>bar</strong> baz</em></p>\n");

    g_autofree gchar *example387 = parse ("_foo _bar_ baz_\n");
    g_assert_cmpstr (example387, ==, "<p><em>foo <em>bar</em> baz</em></p>\n");

    g_autofree gchar *example388 = parse ("__foo_ bar_\n");
    g_assert_cmpstr (example388, ==, "<p><em><em>foo</em> bar</em></p>\n");

    g_autofree gchar *example389 = parse ("*foo *bar**\n");
    g_assert_cmpstr (example389, ==, "<p><em>foo <em>bar</em></em></p>\n");

    g_autofree gchar *example390 = parse ("*foo **bar** baz*\n");
    g_assert_cmpstr (example390, ==, "<p><em>foo <strong>bar</strong> baz</em></p>\n");

    g_autofree gchar *example391 = parse ("*foo**bar**baz*\n");
    g_assert_cmpstr (example391, ==, "<p><em>foo<strong>bar</strong>baz</em></p>\n");

    g_autofree gchar *example392 = parse ("***foo** bar*\n");
    g_assert_cmpstr (example392, ==, "<p><em><strong>foo</strong> bar</em></p>\n");

    g_autofree gchar *example393 = parse ("*foo **bar***\n");
    g_assert_cmpstr (example393, ==, "<p><em>foo <strong>bar</strong></em></p>\n");

    g_autofree gchar *example394 = parse ("*foo**bar***\n");
    g_assert_cmpstr (example394, ==, "<p><em>foo<strong>bar</strong></em></p>\n");

    g_autofree gchar *example395 = parse ("*foo **bar *baz* bim** bop*\n");
    g_assert_cmpstr (example395, ==, "<p><em>foo <strong>bar <em>baz</em> bim</strong> bop</em></p>\n");

    g_autofree gchar *example397 = parse ("** is not an empty emphasis\n");
    g_assert_cmpstr (example397, ==, "<p>** is not an empty emphasis</p>\n");

    g_autofree gchar *example398 = parse ("**** is not an empty strong emphasis\n");
    g_assert_cmpstr (example398, ==, "<p>**** is not an empty strong emphasis</p>\n");

    g_autofree gchar *example400 = parse ("**foo\nbar**\n");
    g_assert_cmpstr (example400, ==, "<p><strong>foo\nbar</strong></p>\n");

    g_autofree gchar *example401 = parse ("__foo _bar_ baz__\n");
    g_assert_cmpstr (example401, ==, "<p><strong>foo <em>bar</em> baz</strong></p>\n");

    g_autofree gchar *example402 = parse ("__foo __bar__ baz__\n");
    g_assert_cmpstr (example402, ==, "<p><strong>foo <strong>bar</strong> baz</strong></p>\n");

    g_autofree gchar *example403 = parse ("____foo__ bar__\n");
    g_assert_cmpstr (example403, ==, "<p><strong><strong>foo</strong> bar</strong></p>\n");

    g_autofree gchar *example404 = parse ("**foo **bar****\n");
    g_assert_cmpstr (example404, ==, "<p><strong>foo <strong>bar</strong></strong></p>\n");

    g_autofree gchar *example405 = parse ("**foo *bar* baz**\n");
    g_assert_cmpstr (example405, ==, "<p><strong>foo <em>bar</em> baz</strong></p>\n");

    g_autofree gchar *example406 = parse ("**foo*bar*baz**\n");
    g_assert_cmpstr (example406, ==, "<p><strong>foo<em>bar</em>baz</strong></p>\n");

    g_autofree gchar *example407 = parse ("***foo* bar**\n");
    g_assert_cmpstr (example407, ==, "<p><strong><em>foo</em> bar</strong></p>\n");

    g_autofree gchar *example408 = parse ("**foo *bar***\n");
    g_assert_cmpstr (example408, ==, "<p><strong>foo <em>bar</em></strong></p>\n");

    g_autofree gchar *example409 = parse ("**foo *bar **baz**\nbim* bop**\n");
    g_assert_cmpstr (example409, ==, "<p><strong>foo <em>bar <strong>baz</strong>\nbim</em> bop</strong></p>\n");

    g_autofree gchar *example411 = parse ("__ is not an empty emphasis\n");
    g_assert_cmpstr (example411, ==, "<p>__ is not an empty emphasis</p>\n");

    g_autofree gchar *example412 = parse ("____ is not an empty strong emphasis\n");
    g_assert_cmpstr (example412, ==, "<p>____ is not an empty strong emphasis</p>\n");

    g_autofree gchar *example413 = parse ("foo ***\n");
    g_assert_cmpstr (example413, ==, "<p>foo ***</p>\n");

    g_autofree gchar *example414 = parse ("foo *\\**\n");
    g_assert_cmpstr (example414, ==, "<p>foo <em>*</em></p>\n");

    g_autofree gchar *example415 = parse ("foo *_*\n");
    g_assert_cmpstr (example415, ==, "<p>foo <em>_</em></p>\n");

    g_autofree gchar *example416 = parse ("foo *****\n");
    g_assert_cmpstr (example416, ==, "<p>foo *****</p>\n");

    g_autofree gchar *example417 = parse ("foo **\\***\n");
    g_assert_cmpstr (example417, ==, "<p>foo <strong>*</strong></p>\n");

    g_autofree gchar *example418 = parse ("foo **_**\n");
    g_assert_cmpstr (example418, ==, "<p>foo <strong>_</strong></p>\n");

    g_autofree gchar *example419 = parse ("**foo*\n");
    g_assert_cmpstr (example419, ==, "<p>*<em>foo</em></p>\n");

    g_autofree gchar *example420 = parse ("*foo**\n");
    g_assert_cmpstr (example420, ==, "<p><em>foo</em>*</p>\n");

    g_autofree gchar *example421 = parse ("***foo**\n");
    g_assert_cmpstr (example421, ==, "<p>*<strong>foo</strong></p>\n");

    g_autofree gchar *example422 = parse ("****foo*\n");
    g_assert_cmpstr (example422, ==, "<p>***<em>foo</em></p>\n");

    g_autofree gchar *example423 = parse ("**foo***\n");
    g_assert_cmpstr (example423, ==, "<p><strong>foo</strong>*</p>\n");

    g_autofree gchar *example424 = parse ("*foo****\n");
    g_assert_cmpstr (example424, ==, "<p><em>foo</em>***</p>\n");

    g_autofree gchar *example425 = parse ("foo ___\n");
    g_assert_cmpstr (example425, ==, "<p>foo ___</p>\n");

    g_autofree gchar *example426 = parse ("foo _\\__\n");
    g_assert_cmpstr (example426, ==, "<p>foo <em>_</em></p>\n");

    g_autofree gchar *example427 = parse ("foo _*_\n");
    g_assert_cmpstr (example427, ==, "<p>foo <em>*</em></p>\n");

    g_autofree gchar *example428 = parse ("foo _____\n");
    g_assert_cmpstr (example428, ==, "<p>foo _____</p>\n");

    g_autofree gchar *example429 = parse ("foo __\\___\n");
    g_assert_cmpstr (example429, ==, "<p>foo <strong>_</strong></p>\n");

    g_autofree gchar *example430 = parse ("foo __*__\n");
    g_assert_cmpstr (example430, ==, "<p>foo <strong>*</strong></p>\n");

    g_autofree gchar *example431 = parse ("__foo_\n");
    g_assert_cmpstr (example431, ==, "<p>_<em>foo</em></p>\n");

    g_autofree gchar *example432 = parse ("_foo__\n");
    g_assert_cmpstr (example432, ==, "<p><em>foo</em>_</p>\n");

    g_autofree gchar *example433 = parse ("___foo__\n");
    g_assert_cmpstr (example433, ==, "<p>_<strong>foo</strong></p>\n");

    g_autofree gchar *example434 = parse ("____foo_\n");
    g_assert_cmpstr (example434, ==, "<p>___<em>foo</em></p>\n");

    g_autofree gchar *example435 = parse ("__foo___\n");
    g_assert_cmpstr (example435, ==, "<p><strong>foo</strong>_</p>\n");

    g_autofree gchar *example436 = parse ("_foo____\n");
    g_assert_cmpstr (example436, ==, "<p><em>foo</em>___</p>\n");

    g_autofree gchar *example437 = parse ("**foo**\n");
    g_assert_cmpstr (example437, ==, "<p><strong>foo</strong></p>\n");

    g_autofree gchar *example438 = parse ("*_foo_*\n");
    g_assert_cmpstr (example438, ==, "<p><em><em>foo</em></em></p>\n");

    g_autofree gchar *example439 = parse ("__foo__\n");
    g_assert_cmpstr (example439, ==, "<p><strong>foo</strong></p>\n");

    g_autofree gchar *example440 = parse ("_*foo*_\n");
    g_assert_cmpstr (example440, ==, "<p><em><em>foo</em></em></p>\n");

    g_autofree gchar *example441 = parse ("****foo****\n");
    g_assert_cmpstr (example441, ==, "<p><strong><strong>foo</strong></strong></p>\n");

    g_autofree gchar *example442 = parse ("____foo____\n");
    g_assert_cmpstr (example442, ==, "<p><strong><strong>foo</strong></strong></p>\n");

    g_autofree gchar *example443 = parse ("******foo******\n");
    g_assert_cmpstr (example443, ==, "<p><strong><strong><strong>foo</strong></strong></strong></p>\n");

    g_autofree gchar *example444 = parse ("***foo***\n");
    g_assert_cmpstr (example444, ==, "<p><em><strong>foo</strong></em></p>\n");

    g_autofree gchar *example445 = parse ("_____foo_____\n");
    g_assert_cmpstr (example445, ==, "<p><em><strong><strong>foo</strong></strong></em></p>\n");

    g_autofree gchar *example446 = parse ("*foo _bar* baz_\n");
    g_assert_cmpstr (example446, ==, "<p><em>foo _bar</em> baz_</p>\n");

    g_autofree gchar *example447 = parse ("*foo __bar *baz bim__ bam*\n");
    g_assert_cmpstr (example447, ==, "<p><em>foo <strong>bar *baz bim</strong> bam</em></p>\n");

    g_autofree gchar *example448 = parse ("**foo **bar baz**\n");
    g_assert_cmpstr (example448, ==, "<p>**foo <strong>bar baz</strong></p>\n");

    g_autofree gchar *example449 = parse ("*foo *bar baz*\n");
    g_assert_cmpstr (example449, ==, "<p>*foo <em>bar baz</em></p>\n");

    g_autofree gchar *example455 = parse ("*a `*`*\n");
    g_assert_cmpstr (example455, ==, "<p><em>a <code>*</code></em></p>\n");

    g_autofree gchar *example456 = parse ("_a `_`_\n");
    g_assert_cmpstr (example456, ==, "<p><em>a <code>_</code></em></p>\n");
}

static void
test_markdown_textual_content (void)
{
    g_autofree gchar *example622 = parse ("hello $.;'there\n");
    g_assert_cmpstr (example622, ==, "<p>hello $.;'there</p>\n");

    g_autofree gchar *example623 = parse ("Foo χρῆν\n");
    g_assert_cmpstr (example623, ==, "<p>Foo χρῆν</p>\n");

    g_autofree gchar *example624 = parse ("Multiple     spaces\n");
    g_assert_cmpstr (example624, ==, "<p>Multiple     spaces</p>\n");
}

static gchar *serialize_url_node (SnapdMarkdownNode *node);

static gchar *
serialize_url_nodes (GPtrArray *nodes)
{
    g_autoptr(GString) text = g_string_new ("");
    for (int i = 0; i < nodes->len; i++) {
        SnapdMarkdownNode *node = g_ptr_array_index (nodes, i);
        g_autofree gchar *node_text = serialize_url_node (node);
        g_string_append (text, node_text);
    }

    return g_steal_pointer (&text->str);
}

static gchar *
serialize_url_node (SnapdMarkdownNode *node)
{
   GPtrArray *children = snapd_markdown_node_get_children (node);

   g_autofree gchar *contents = NULL;
   switch (snapd_markdown_node_get_node_type (node)) {
   case SNAPD_MARKDOWN_NODE_TYPE_TEXT:
       return g_strdup (snapd_markdown_node_get_text (node));

   case SNAPD_MARKDOWN_NODE_TYPE_PARAGRAPH:
       contents = serialize_url_nodes (children);
       return g_strdup_printf ("<p>%s</p>", contents);

   case SNAPD_MARKDOWN_NODE_TYPE_URL:
       contents = serialize_url_nodes (children);
       return g_strdup_printf ("<url>%s</url>", contents);

   default:
       g_assert_not_reached ();
       return NULL;
   }
}

static gchar *
parse_url (const gchar *text)
{
    g_autoptr(SnapdMarkdownParser) parser = snapd_markdown_parser_new (SNAPD_MARKDOWN_VERSION_0);
    g_autoptr(GPtrArray) nodes = snapd_markdown_parser_parse (parser, text);
    return serialize_url_nodes (nodes);
}

static void
test_markdown_urls (void)
{
    g_autofree gchar *url0 = parse_url ("http://localhost");
    g_assert_cmpstr (url0, ==, "<p><url>http://localhost</url></p>");

    g_autofree gchar *url1 = parse_url ("https://localhost");
    g_assert_cmpstr (url1, ==, "<p><url>https://localhost</url></p>");

    g_autofree gchar *url2 = parse_url ("mailto:name@example.com");
    g_assert_cmpstr (url2, ==, "<p><url>mailto:name@example.com</url></p>");

    g_autofree gchar *url3 = parse_url ("ftp://foo");
    g_assert_cmpstr (url3, ==, "<p>ftp://foo</p>");

    g_autofree gchar *url4 = parse_url ("http://");
    g_assert_cmpstr (url4, ==, "<p>http://</p>");

    g_autofree gchar *url5 = parse_url ("https://");
    g_assert_cmpstr (url5, ==, "<p>https://</p>");

    g_autofree gchar *url6 = parse_url ("mailto:");
    g_assert_cmpstr (url6, ==, "<p>mailto:</p>");

    g_autofree gchar *url7 = parse_url (" https://localhost");
    g_assert_cmpstr (url7, ==, "<p><url>https://localhost</url></p>");

    g_autofree gchar *url8 = parse_url ("https://localhost ");
    g_assert_cmpstr (url8, ==, "<p><url>https://localhost</url></p>");

    g_autofree gchar *url9 = parse_url (" https://localhost ");
    g_assert_cmpstr (url9, ==, "<p><url>https://localhost</url></p>");

    g_autofree gchar *url10 = parse_url ("x https://localhost");
    g_assert_cmpstr (url10, ==, "<p>x <url>https://localhost</url></p>");

    g_autofree gchar *url11 = parse_url ("https://localhost x");
    g_assert_cmpstr (url11, ==, "<p><url>https://localhost</url> x</p>");

    g_autofree gchar *url12 = parse_url ("x https://localhost x");
    g_assert_cmpstr (url12, ==, "<p>x <url>https://localhost</url> x</p>");

    g_autofree gchar *url13 = parse_url ("(https://localhost)");
    g_assert_cmpstr (url13, ==, "<p>(<url>https://localhost</url>)</p>");

    g_autofree gchar *url14 = parse_url ("https://localhost/(foo)");
    g_assert_cmpstr (url14, ==, "<p><url>https://localhost/(foo)</url></p>");

    g_autofree gchar *url15 = parse_url ("https://localhost/.");
    g_assert_cmpstr (url15, ==, "<p><url>https://localhost/</url>.</p>");

    g_autofree gchar *url16 = parse_url ("https://localhost/,");
    g_assert_cmpstr (url16, ==, "<p><url>https://localhost/</url>,</p>");
}

static gchar *
parse_whitespace (const gchar *text)
{
    g_autoptr(SnapdMarkdownParser) parser = snapd_markdown_parser_new (SNAPD_MARKDOWN_VERSION_0);
    g_assert_false (snapd_markdown_parser_get_preserve_whitespace (parser));
    g_autoptr(GPtrArray) nodes = snapd_markdown_parser_parse (parser, text);
    return serialize_nodes (nodes);
}

static void
test_markdown_whitespace (void)
{
    g_autofree gchar *whitespace0 = parse_whitespace ("Inter  word");
    g_assert_cmpstr (whitespace0, ==, "<p>Inter word</p>\n");

    g_autofree gchar *whitespace1 = parse_whitespace ("Inter    word");
    g_assert_cmpstr (whitespace1, ==, "<p>Inter word</p>\n");

    g_autofree gchar *whitespace2 = parse_whitespace ("New\nline");
    g_assert_cmpstr (whitespace2, ==, "<p>New line</p>\n");

    g_autofree gchar *whitespace3 = parse_whitespace ("New \n line");
    g_assert_cmpstr (whitespace3, ==, "<p>New line</p>\n");

    g_autofree gchar *whitespace4 = parse_whitespace ("A  *very  emphasised*  line");
    g_assert_cmpstr (whitespace4, ==, "<p>A <em>very emphasised</em> line</p>\n");
}

int
main (int argc, char **argv)
{
    g_test_init (&argc, &argv, NULL);

    g_test_add_func ("/markdown/empty", test_markdown_empty);
    g_test_add_func ("/markdown/single-character", test_markdown_single_character);
    g_test_add_func ("/markdown/precedence", test_markdown_precedence);
    g_test_add_func ("/markdown/indented-code", test_markdown_indented_code);
    g_test_add_func ("/markdown/paragraphs", test_markdown_paragraphs);
    g_test_add_func ("/markdown/list-items", test_markdown_list_items);
    g_test_add_func ("/markdown/lists", test_markdown_lists);
    g_test_add_func ("/markdown/inlines", test_markdown_inlines);
    g_test_add_func ("/markdown/code-spans", test_markdown_code_spans);
    g_test_add_func ("/markdown/emphasis", test_markdown_emphasis);
    g_test_add_func ("/markdown/textual-content", test_markdown_textual_content);
    g_test_add_func ("/markdown/urls", test_markdown_urls);
    g_test_add_func ("/markdown/whitespace", test_markdown_whitespace);

    return g_test_run ();
}
