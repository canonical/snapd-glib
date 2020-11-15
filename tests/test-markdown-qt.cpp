/*
 * Copyright (C) 2019 Canonical Ltd.
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License as published by the Free
 * Software Foundation; either version 2 or version 3 of the License.
 * See http://www.gnu.org/copyleft/lgpl.html the full text of the license.
 */

#include <glib-object.h>

#include <Snapd/MarkdownParser>
#include <QDebug>

static QString
escape_text (const QString &text)
{
    QString escaped_text;

    for (int i = 0; i < text.size (); i++) {
        if (text[i] == '&')
            escaped_text += "&amp;";
        else if (text[i] == '<')
            escaped_text += "&lt;";
        else if (text[i] == '>')
            escaped_text += "&gt;";
        else if (text[i] == '"')
            escaped_text += "&quot;";
        else
            escaped_text += text[i];
    }

    return escaped_text;
}

static QString serialize_node (QSnapdMarkdownNode &node);

static QString
serialize_children (QSnapdMarkdownNode &node)
{
    QString result;
    for (int i = 0; i < node.childCount (); i++) {
        QScopedPointer<QSnapdMarkdownNode> child (node.child (i));
        result += serialize_node (*child);
    }
    return result;
}

static QString
serialize_node (QSnapdMarkdownNode &node)
{
   switch (node.type ()) {
   case QSnapdMarkdownNode::NodeTypeText:
       return escape_text (node.text ());

   case QSnapdMarkdownNode::NodeTypeParagraph:
       return "<p>" + serialize_children (node) + "</p>\n";

   case QSnapdMarkdownNode::NodeTypeUnorderedList:
       return "<ul>\n" + serialize_children (node) + "</ul>\n";

   case QSnapdMarkdownNode::NodeTypeListItem:
       if (node.childCount () == 0)
           return "<li></li>\n";
       if (node.childCount () == 1) {
           QScopedPointer<QSnapdMarkdownNode> child (node.child (0));
           if (child->type () == QSnapdMarkdownNode::NodeTypeParagraph)
               return "<li>" + serialize_children (*child) + "</li>\n";
       }
       return "<li>\n" + serialize_children (node) + "</li>\n";

   case QSnapdMarkdownNode::NodeTypeCodeBlock:
       return "<pre><code>" + serialize_children (node) + "</code></pre>\n";

   case QSnapdMarkdownNode::NodeTypeCodeSpan:
       return "<code>" + serialize_children (node) + "</code>";

   case QSnapdMarkdownNode::NodeTypeEmphasis:
       return "<em>" + serialize_children (node) + "</em>";

   case QSnapdMarkdownNode::NodeTypeStrongEmphasis:
       return "<strong>" + serialize_children (node) + "</strong>";

   case QSnapdMarkdownNode::NodeTypeUrl:
       return serialize_children (node);

   default:
       g_assert_not_reached ();
       return "";
   }
}

static QString
parse (const QString &text)
{
    QSnapdMarkdownParser parser (QSnapdMarkdownParser::MarkdownVersion0);
    parser.setPreserveWhitespace (true);
    QList<QSnapdMarkdownNode> nodes = parser.parse (text);
    QString result;
    for (int i = 0; i < nodes.size (); i++)
        result += serialize_node (nodes[i]);
    return result;
}

static void
test_markdown_empty ()
{
    QString markup = parse ("");
    g_assert_true (markup == "");
}

static void
test_markdown_single_character ()
{
    QString markup = parse ("a");
    g_assert_true (markup == "<p>a</p>\n");
}

/* The following tests are a subset of those in the CommonMark spec (https://spec.commonmark.org/0.28).
 * Some tests are modified to match the expected snap behaviour */

static void
test_markdown_precedence ()
{
    QString example12 = parse ("- `one\n- two`\n");
    g_assert_true (example12 == "<ul>\n<li>`one</li>\n<li>two`</li>\n</ul>\n");
}

static void
test_markdown_indented_code ()
{
    QString example76 = parse ("    a simple\n      indented code block\n");
    g_assert_true (example76 == "<pre><code>a simple\n  indented code block\n</code></pre>\n");

    QString example77 = parse ("  - foo\n\n    bar\n");
    g_assert_true (example77 == "<ul>\n<li>\n<p>foo</p>\n<p>bar</p>\n</li>\n</ul>\n");

    QString example79 = parse ("    <a/>\n    *hi*\n\n    - one\n");
    g_assert_true (example79 == "<pre><code>&lt;a/&gt;\n*hi*\n\n- one\n</code></pre>\n");

    QString example80 = parse ("    chunk1\n\n    chunk2\n  \n \n \n    chunk3\n");
    g_assert_true (example80 == "<pre><code>chunk1\n\nchunk2\n\n\n\nchunk3\n</code></pre>\n");

    QString example81 = parse ("    chunk1\n      \n      chunk2\n");
    g_assert_true (example81 == "<pre><code>chunk1\n  \n  chunk2\n</code></pre>\n");

    QString example82 = parse ("Foo\n    bar\n\n");
    g_assert_true (example82 == "<p>Foo\nbar</p>\n");

    QString example83 = parse ("    foo\nbar\n");
    g_assert_true (example83 == "<pre><code>foo\n</code></pre>\n<p>bar</p>\n");

    QString example85 = parse ("        foo\n    bar\n");
    g_assert_true (example85 == "<pre><code>    foo\nbar\n</code></pre>\n");

    QString example86 = parse ("\n    \n    foo\n    \n\n");
    g_assert_true (example86 == "<pre><code>foo\n</code></pre>\n");

    QString example87 = parse ("    foo  \n");
    g_assert_true (example87 == "<pre><code>foo  \n</code></pre>\n");
}

static void
test_markdown_paragraphs ()
{
    QString example182 = parse ("aaa\n\nbbb\n");
    g_assert_true (example182 == "<p>aaa</p>\n<p>bbb</p>\n");

    QString example183 = parse ("aaa\nbbb\n\nccc\nddd\n");
    g_assert_true (example183 == "<p>aaa\nbbb</p>\n<p>ccc\nddd</p>\n");

    QString example184 = parse ("aaa\n\n\nbbb\n");
    g_assert_true (example184 == "<p>aaa</p>\n<p>bbb</p>\n");

    QString example185 = parse ("  aaa\n bbb\n");
    g_assert_true (example185 == "<p>aaa\nbbb</p>\n");

    QString example186 = parse ("aaa\n             bbb\n                                       ccc\n");
    g_assert_true (example186 == "<p>aaa\nbbb\nccc</p>\n");

    QString example187 = parse ("   aaa\nbbb\n");
    g_assert_true (example187 == "<p>aaa\nbbb</p>\n");

    QString example188 = parse ("    aaa\nbbb\n");
    g_assert_true (example188 == "<pre><code>aaa\n</code></pre>\n<p>bbb</p>\n");
}

static void
test_markdown_list_items ()
{
    QString example218 = parse ("- one\n\n two\n");
    g_assert_true (example218 == "<ul>\n<li>one</li>\n</ul>\n<p>two</p>\n");

    QString example219 = parse ("- one\n\n  two\n");
    g_assert_true (example219 == "<ul>\n<li>\n<p>one</p>\n<p>two</p>\n</li>\n</ul>\n");

    QString example220 = parse (" -    one\n\n     two\n");
    g_assert_true (example220 == "<ul>\n<li>one</li>\n</ul>\n<pre><code> two\n</code></pre>\n");

    QString example221 = parse (" -    one\n\n      two\n");
    g_assert_true (example221 == "<ul>\n<li>\n<p>one</p>\n<p>two</p>\n</li>\n</ul>\n");

    QString example224 = parse ("-one\n\n2.two\n");
    g_assert_true (example224 == "<p>-one</p>\n<p>2.two</p>\n");

    QString example225 = parse ("- foo\n\n\n  bar\n");
    g_assert_true (example225 == "<ul>\n<li>\n<p>foo</p>\n<p>bar</p>\n</li>\n</ul>\n");

    QString example227 = parse ("- Foo\n\n      bar\n\n\n      baz\n");
    g_assert_true (example227 == "<ul>\n<li>\n<p>Foo</p>\n<pre><code>bar\n\n\nbaz\n</code></pre>\n</li>\n</ul>\n");

    QString example229 = parse ("1234567890. not ok\n");
    g_assert_true (example229 == "<p>1234567890. not ok</p>\n");

    QString example232 = parse ("-1. not ok\n");
    g_assert_true (example232 == "<p>-1. not ok</p>\n");

    QString example233 = parse ("- foo\n\n      bar\n");
    g_assert_true (example233 == "<ul>\n<li>\n<p>foo</p>\n<pre><code>bar\n</code></pre>\n</li>\n</ul>\n");

    QString example235 = parse ("    indented code\n\nparagraph\n\n    more code\n");
    g_assert_true (example235 == "<pre><code>indented code\n</code></pre>\n<p>paragraph</p>\n<pre><code>more code\n</code></pre>\n");

    QString example238 = parse ("   foo\n\nbar\n");
    g_assert_true (example238 == "<p>foo</p>\n<p>bar</p>\n");

    QString example239 = parse ("-    foo\n\n  bar\n");
    g_assert_true (example239 == "<ul>\n<li>foo</li>\n</ul>\n<p>bar</p>\n");

    QString example240 = parse ("-  foo\n\n   bar\n");
    g_assert_true (example240 == "<ul>\n<li>\n<p>foo</p>\n<p>bar</p>\n</li>\n</ul>\n");

    QString example242 = parse ("-   \n  foo\n");
    g_assert_true (example242 == "<ul>\n<li>foo</li>\n</ul>\n");

    QString example243 = parse ("-\n\n  foo\n");
    g_assert_true (example243 == "<ul>\n<li></li>\n</ul>\n<p>foo</p>\n");

    QString example244 = parse ("- foo\n-\n- bar\n");
    g_assert_true (example244 == "<ul>\n<li>foo</li>\n<li></li>\n<li>bar</li>\n</ul>\n");

    QString example245 = parse ("- foo\n-   \n- bar\n");
    g_assert_true (example245 == "<ul>\n<li>foo</li>\n<li></li>\n<li>bar</li>\n</ul>\n");

    QString example247 = parse ("*\n");
    g_assert_true (example247 == "<ul>\n<li></li>\n</ul>\n");

    QString example248 = parse ("foo\n*\n\nfoo\n1.\n");
    g_assert_true (example248 == "<p>foo\n*</p>\n<p>foo\n1.</p>\n");

    QString example252 = parse ("    1.  A paragraph\n        with two lines.\n\n            indented code\n\n        > A block quote.\n");
    g_assert_true (example252 == "<pre><code>1.  A paragraph\n    with two lines.\n\n        indented code\n\n    &gt; A block quote.\n</code></pre>\n");

    QString example257 = parse ("- foo\n  - bar\n    - baz\n      - boo\n");
    g_assert_true (example257 == "<ul>\n<li>\n<p>foo</p>\n<ul>\n<li>\n<p>bar</p>\n<ul>\n<li>\n<p>baz</p>\n<ul>\n<li>boo</li>\n</ul>\n</li>\n</ul>\n</li>\n</ul>\n</li>\n</ul>\n");

    QString example258 = parse ("- foo\n - bar\n  - baz\n   - boo\n");
    g_assert_true (example258 == "<ul>\n<li>foo</li>\n<li>bar</li>\n<li>baz</li>\n<li>boo</li>\n</ul>\n");

    QString example261 = parse ("- - foo\n");
    g_assert_true (example261 == "<ul>\n<li>\n<ul>\n<li>foo</li>\n</ul>\n</li>\n</ul>\n");
}

static void
test_markdown_lists ()
{
    QString example264 = parse ("- foo\n- bar\n+ baz\n");
    g_assert_true (example264 == "<ul>\n<li>foo</li>\n<li>bar</li>\n</ul>\n<ul>\n<li>baz</li>\n</ul>\n");

    QString example266 = parse ("Foo\n- bar\n- baz\n");
    g_assert_true (example266 == "<p>Foo</p>\n<ul>\n<li>bar</li>\n<li>baz</li>\n</ul>\n");

    QString example267 = parse ("The number of windows in my house is\n14.  The number of doors is 6.\n");
    g_assert_true (example267 == "<p>The number of windows in my house is\n14.  The number of doors is 6.</p>\n");

    QString example269 = parse ("- foo\n\n- bar\n\n\n- baz\n");
    g_assert_true (example269 == "<ul>\n<li>foo</li>\n<li>bar</li>\n<li>baz</li>\n</ul>\n");

    QString example270 = parse ("- foo\n  - bar\n    - baz\n\n\n      bim\n");
    g_assert_true (example270 == "<ul>\n<li>\n<p>foo</p>\n<ul>\n<li>\n<p>bar</p>\n<ul>\n<li>\n<p>baz</p>\n<p>bim</p>\n</li>\n</ul>\n</li>\n</ul>\n</li>\n</ul>\n");

    QString example273 = parse ("- a\n - b\n  - c\n   - d\n    - e\n   - f\n  - g\n - h\n- i\n");
    g_assert_true (example273 == "<ul>\n<li>a</li>\n<li>b</li>\n<li>c</li>\n<li>d</li>\n<li>e</li>\n<li>f</li>\n<li>g</li>\n<li>h</li>\n<li>i</li>\n</ul>\n");

    QString example275 = parse ("- a\n- b\n\n- c\n");
    g_assert_true (example275 == "<ul>\n<li>a</li>\n<li>b</li>\n<li>c</li>\n</ul>\n");

    QString example276 = parse ("* a\n*\n\n* c\n");
    g_assert_true (example276 == "<ul>\n<li>a</li>\n<li></li>\n<li>c</li>\n</ul>\n");

    QString example277 = parse ("- a\n- b\n\n  c\n- d\n");
    g_assert_true (example277 == "<ul>\n<li>a</li>\n<li>\n<p>b</p>\n<p>c</p>\n</li>\n<li>d</li>\n</ul>\n");

    QString example280 = parse ("- a\n  - b\n\n    c\n- d\n");
    g_assert_true (example280 == "<ul>\n<li>\n<p>a</p>\n<ul>\n<li>\n<p>b</p>\n<p>c</p>\n</li>\n</ul>\n</li>\n<li>d</li>\n</ul>\n");

    QString example283 = parse ("- a\n");
    g_assert_true (example283 == "<ul>\n<li>a</li>\n</ul>\n");

    QString example284 = parse ("- a\n  - b\n");
    g_assert_true (example284 == "<ul>\n<li>\n<p>a</p>\n<ul>\n<li>b</li>\n</ul>\n</li>\n</ul>\n");

    QString example286 = parse ("* foo\n  * bar\n\n  baz\n");
    g_assert_true (example286 == "<ul>\n<li>\n<p>foo</p>\n<ul>\n<li>bar</li>\n</ul>\n<p>baz</p>\n</li>\n</ul>\n");

    QString example287 = parse ("- a\n  - b\n  - c\n\n- d\n  - e\n  - f\n");
    g_assert_true (example287 == "<ul>\n<li>\n<p>a</p>\n<ul>\n<li>b</li>\n<li>c</li>\n</ul>\n</li>\n<li>\n<p>d</p>\n<ul>\n<li>e</li>\n<li>f</li>\n</ul>\n</li>\n</ul>\n");
}

static void
test_markdown_inlines ()
{
    QString example288 = parse ("`hi`lo`\n");
    g_assert_true (example288 == "<p><code>hi</code>lo`</p>\n");

    QString example289 = parse ("\\!\\\"\\#\\$\\%\\&\\'\\(\\)\\*\\+\\,\\-\\.\\/\\:\\;\\<\\=\\>\\?\\@\\[\\\\\\]\\^\\_\\`\\{\\|\\}\\~\n");
    g_assert_true (example289 == "<p>!&quot;#$%&amp;'()*+,-./:;&lt;=&gt;?@[\\]^_`{|}~</p>\n");

    QString example290 = parse ("\\\t\\A\\a\\ \\3\\φ\\«\n");
    g_assert_true (example290 == "<p>\\\t\\A\\a\\ \\3\\φ\\«</p>\n");

    QString example291 = parse ("\\*not emphasized*\n\\<br/> not a tag\n\\[not a link](/foo)\n\\`not code`\n1\\. not a list\n\\* not a list\n\\# not a heading\n\\[foo]: /url \"not a reference\"\n");
    g_assert_true (example291 == "<p>*not emphasized*\n&lt;br/&gt; not a tag\n[not a link](/foo)\n`not code`\n1. not a list\n* not a list\n# not a heading\n[foo]: /url &quot;not a reference&quot;</p>\n");

    QString example292 = parse ("\\\\*emphasis*\n");
    g_assert_true (example292 == "<p>\\<em>emphasis</em></p>\n");

    QString example295 = parse ("    \\[\\]\n");
    g_assert_true (example295 == "<pre><code>\\[\\]\n</code></pre>\n");
}

static void
test_markdown_code_spans ()
{
    QString example314 = parse ("`foo`\n");
    g_assert_true (example314 == "<p><code>foo</code></p>\n");

    QString example315 = parse ("`` foo ` bar  ``\n");
    g_assert_true (example315 == "<p><code>foo ` bar</code></p>\n");

    QString example316 = parse ("` `` `\n");
    g_assert_true (example316 == "<p><code>``</code></p>\n");

    QString example317 = parse ("``\nfoo\n``\n");
    g_assert_true (example317 == "<p><code>foo</code></p>\n");

    QString example318 = parse ("`foo   bar\n  baz`\n");
    g_assert_true (example318 == "<p><code>foo bar baz</code></p>\n");

    QString example319 = parse ("`a  b`\n");
    g_assert_true (example319 == "<p><code>a  b</code></p>\n");

    QString example320 = parse ("`foo `` bar`\n");
    g_assert_true (example320 == "<p><code>foo `` bar</code></p>\n");

    QString example321 = parse ("`foo\\`bar`\n");
    g_assert_true (example321 == "<p><code>foo\\</code>bar`</p>\n");

    QString example322 = parse ("*foo`*`\n");
    g_assert_true (example322 == "<p>*foo<code>*</code></p>\n");

    QString example326 = parse ("`<http://foo.bar.`baz>`\n");
    g_assert_true (example326 == "<p><code>&lt;http://foo.bar.</code>baz&gt;`</p>\n");

    QString example328 = parse ("```foo``\n");
    g_assert_true (example328 == "<p>```foo``</p>\n");

    QString example329 = parse ("`foo\n");
    g_assert_true (example329 == "<p>`foo</p>\n");

    QString example330 = parse ("`foo``bar``\n");
    g_assert_true (example330 == "<p>`foo<code>bar</code></p>\n");
}

static void
test_markdown_emphasis ()
{
    QString example331 = parse ("*foo bar*\n");
    g_assert_true (example331 == "<p><em>foo bar</em></p>\n");

    QString example332 = parse ("a * foo bar*\n");
    g_assert_true (example332 == "<p>a * foo bar*</p>\n");

    QString example333 = parse ("a*\"foo\"*\n");
    g_assert_true (example333 == "<p>a*&quot;foo&quot;*</p>\n");

    QString example334 = parse ("* a *\n");
    //FIXME g_assert_true (example334 == "<p>* a *</p>\n");

    QString example335 = parse ("foo*bar*\n");
    g_assert_true (example335 == "<p>foo<em>bar</em></p>\n");

    QString example336 = parse ("5*6*78\n");
    g_assert_true (example336 == "<p>5<em>6</em>78</p>\n");

    QString example337 = parse ("_foo bar_\n");
    g_assert_true (example337 == "<p><em>foo bar</em></p>\n");

    QString example338 = parse ("_ foo bar_\n");
    g_assert_true (example338 == "<p>_ foo bar_</p>\n");

    QString example339 = parse ("a_\"foo\"_\n");
    g_assert_true (example339 == "<p>a_&quot;foo&quot;_</p>\n");

    QString example340 = parse ("foo_bar_\n");
    g_assert_true (example340 == "<p>foo_bar_</p>\n");

    QString example341 = parse ("5_6_78\n");
    g_assert_true (example341 == "<p>5_6_78</p>\n");

    QString example342 = parse ("пристаням_стремятся_\n");
    g_assert_true (example342 == "<p>пристаням_стремятся_</p>\n");

    QString example343 = parse ("aa_\"bb\"_cc\n");
    g_assert_true (example343 == "<p>aa_&quot;bb&quot;_cc</p>\n");

    QString example344 = parse ("foo-_(bar)_\n");
    g_assert_true (example344 == "<p>foo-<em>(bar)</em></p>\n");

    QString example345 = parse ("_foo*\n");
    g_assert_true (example345 == "<p>_foo*</p>\n");

    QString example346 = parse ("*foo bar *\n");
    g_assert_true (example346 == "<p>*foo bar *</p>\n");

    QString example347 = parse ("*foo bar\n*\n");
    g_assert_true (example347 == "<p>*foo bar\n*</p>\n");

    QString example348 = parse ("*(*foo)\n");
    g_assert_true (example348 == "<p>*(*foo)</p>\n");

    QString example349 = parse ("*(*foo*)*\n");
    g_assert_true (example349 == "<p><em>(<em>foo</em>)</em></p>\n");

    QString example350 = parse ("*foo*bar\n");
    g_assert_true (example350 == "<p><em>foo</em>bar</p>\n");

    QString example351 = parse ("_foo bar _\n");
    g_assert_true (example351 == "<p>_foo bar _</p>\n");

    QString example352 = parse ("_(_foo)\n");
    g_assert_true (example352 == "<p>_(_foo)</p>\n");

    QString example353 = parse ("_(_foo_)_\n");
    g_assert_true (example353 == "<p><em>(<em>foo</em>)</em></p>\n");

    QString example354 = parse ("_foo_bar\n");
    g_assert_true (example354 == "<p>_foo_bar</p>\n");

    QString example355 = parse ("_пристаням_стремятся\n");
    g_assert_true (example355 == "<p>_пристаням_стремятся</p>\n");

    QString example356 = parse ("_foo_bar_baz_\n");
    g_assert_true (example356 == "<p><em>foo_bar_baz</em></p>\n");

    QString example357 = parse ("_(bar)_.\n");
    g_assert_true (example357 == "<p><em>(bar)</em>.</p>\n");

    QString example358 = parse ("**foo bar**\n");
    g_assert_true (example358 == "<p><strong>foo bar</strong></p>\n");

    QString example359 = parse ("** foo bar**\n");
    g_assert_true (example359 == "<p>** foo bar**</p>\n");

    QString example360 = parse ("a**\"foo\"**\n");
    g_assert_true (example360 == "<p>a**&quot;foo&quot;**</p>\n");

    QString example361 = parse ("foo**bar**\n");
    g_assert_true (example361 == "<p>foo<strong>bar</strong></p>\n");

    QString example362 = parse ("__foo bar__\n");
    g_assert_true (example362 == "<p><strong>foo bar</strong></p>\n");

    QString example363 = parse ("__ foo bar__\n");
    g_assert_true (example363 == "<p>__ foo bar__</p>\n");

    QString example364 = parse ("__\nfoo bar__\n");
    g_assert_true (example364 == "<p>__\nfoo bar__</p>\n");

    QString example365 = parse ("a__\"foo\"__\n");
    g_assert_true (example365 == "<p>a__&quot;foo&quot;__</p>\n");

    QString example366 = parse ("foo__bar__\n");
    g_assert_true (example366 == "<p>foo__bar__</p>\n");

    QString example367 = parse ("5__6__78\n");
    g_assert_true (example367 == "<p>5__6__78</p>\n");

    QString example368 = parse ("пристаням__стремятся__\n");
    g_assert_true (example368 == "<p>пристаням__стремятся__</p>\n");

    QString example369 = parse ("__foo, __bar__, baz__\n");
    g_assert_true (example369 == "<p><strong>foo, <strong>bar</strong>, baz</strong></p>\n");

    QString example370 = parse ("foo-__(bar)__\n");
    g_assert_true (example370 == "<p>foo-<strong>(bar)</strong></p>\n");

    QString example371 = parse ("**foo bar **\n");
    g_assert_true (example371 == "<p>**foo bar **</p>\n");

    QString example372 = parse ("**(**foo)\n");
    g_assert_true (example372 == "<p>**(**foo)</p>\n");

    QString example373 = parse ("*(**foo**)*\n");
    g_assert_true (example373 == "<p><em>(<strong>foo</strong>)</em></p>\n");

    QString example374 = parse ("**Gomphocarpus (*Gomphocarpus physocarpus*, syn.\n*Asclepias physocarpa*)**\n");
    g_assert_true (example374 == "<p><strong>Gomphocarpus (<em>Gomphocarpus physocarpus</em>, syn.\n<em>Asclepias physocarpa</em>)</strong></p>\n");

    QString example375 = parse ("**foo \"*bar*\" foo**\n");
    g_assert_true (example375 == "<p><strong>foo &quot;<em>bar</em>&quot; foo</strong></p>\n");

    QString example376 = parse ("**foo**bar\n");
    g_assert_true (example376 == "<p><strong>foo</strong>bar</p>\n");

    QString example377 = parse ("__foo bar __\n");
    g_assert_true (example377 == "<p>__foo bar __</p>\n");

    QString example378 = parse ("__(__foo)\n");
    g_assert_true (example378 == "<p>__(__foo)</p>\n");

    QString example379 = parse ("_(__foo__)_\n");
    g_assert_true (example379 == "<p><em>(<strong>foo</strong>)</em></p>\n");

    QString example380 = parse ("__foo__bar\n");
    g_assert_true (example380 == "<p>__foo__bar</p>\n");

    QString example381 = parse ("__пристаням__стремятся\n");
    g_assert_true (example381 == "<p>__пристаням__стремятся</p>\n");

    QString example382 = parse ("__foo__bar__baz__\n");
    g_assert_true (example382 == "<p><strong>foo__bar__baz</strong></p>\n");

    QString example383 = parse ("__(bar)__.\n");
    g_assert_true (example383 == "<p><strong>(bar)</strong>.</p>\n");

    QString example385 = parse ("*foo\nbar*\n");
    g_assert_true (example385 == "<p><em>foo\nbar</em></p>\n");

    QString example386 = parse ("_foo __bar__ baz_\n");
    g_assert_true (example386 == "<p><em>foo <strong>bar</strong> baz</em></p>\n");

    QString example387 = parse ("_foo _bar_ baz_\n");
    g_assert_true (example387 == "<p><em>foo <em>bar</em> baz</em></p>\n");

    QString example388 = parse ("__foo_ bar_\n");
    g_assert_true (example388 == "<p><em><em>foo</em> bar</em></p>\n");

    QString example389 = parse ("*foo *bar**\n");
    g_assert_true (example389 == "<p><em>foo <em>bar</em></em></p>\n");

    QString example390 = parse ("*foo **bar** baz*\n");
    g_assert_true (example390 == "<p><em>foo <strong>bar</strong> baz</em></p>\n");

    QString example391 = parse ("*foo**bar**baz*\n");
    g_assert_true (example391 == "<p><em>foo<strong>bar</strong>baz</em></p>\n");

    QString example392 = parse ("***foo** bar*\n");
    g_assert_true (example392 == "<p><em><strong>foo</strong> bar</em></p>\n");

    QString example393 = parse ("*foo **bar***\n");
    g_assert_true (example393 == "<p><em>foo <strong>bar</strong></em></p>\n");

    QString example394 = parse ("*foo**bar***\n");
    g_assert_true (example394 == "<p><em>foo<strong>bar</strong></em></p>\n");

    QString example395 = parse ("*foo **bar *baz* bim** bop*\n");
    g_assert_true (example395 == "<p><em>foo <strong>bar <em>baz</em> bim</strong> bop</em></p>\n");

    QString example397 = parse ("** is not an empty emphasis\n");
    g_assert_true (example397 == "<p>** is not an empty emphasis</p>\n");

    QString example398 = parse ("**** is not an empty strong emphasis\n");
    g_assert_true (example398 == "<p>**** is not an empty strong emphasis</p>\n");

    QString example400 = parse ("**foo\nbar**\n");
    g_assert_true (example400 == "<p><strong>foo\nbar</strong></p>\n");

    QString example401 = parse ("__foo _bar_ baz__\n");
    g_assert_true (example401 == "<p><strong>foo <em>bar</em> baz</strong></p>\n");

    QString example402 = parse ("__foo __bar__ baz__\n");
    g_assert_true (example402 == "<p><strong>foo <strong>bar</strong> baz</strong></p>\n");

    QString example403 = parse ("____foo__ bar__\n");
    g_assert_true (example403 == "<p><strong><strong>foo</strong> bar</strong></p>\n");

    QString example404 = parse ("**foo **bar****\n");
    g_assert_true (example404 == "<p><strong>foo <strong>bar</strong></strong></p>\n");

    QString example405 = parse ("**foo *bar* baz**\n");
    g_assert_true (example405 == "<p><strong>foo <em>bar</em> baz</strong></p>\n");

    QString example406 = parse ("**foo*bar*baz**\n");
    g_assert_true (example406 == "<p><strong>foo<em>bar</em>baz</strong></p>\n");

    QString example407 = parse ("***foo* bar**\n");
    g_assert_true (example407 == "<p><strong><em>foo</em> bar</strong></p>\n");

    QString example408 = parse ("**foo *bar***\n");
    g_assert_true (example408 == "<p><strong>foo <em>bar</em></strong></p>\n");

    QString example409 = parse ("**foo *bar **baz**\nbim* bop**\n");
    g_assert_true (example409 == "<p><strong>foo <em>bar <strong>baz</strong>\nbim</em> bop</strong></p>\n");

    QString example411 = parse ("__ is not an empty emphasis\n");
    g_assert_true (example411 == "<p>__ is not an empty emphasis</p>\n");

    QString example412 = parse ("____ is not an empty strong emphasis\n");
    g_assert_true (example412 == "<p>____ is not an empty strong emphasis</p>\n");

    QString example413 = parse ("foo ***\n");
    g_assert_true (example413 == "<p>foo ***</p>\n");

    QString example414 = parse ("foo *\\**\n");
    g_assert_true (example414 == "<p>foo <em>*</em></p>\n");

    QString example415 = parse ("foo *_*\n");
    g_assert_true (example415 == "<p>foo <em>_</em></p>\n");

    QString example416 = parse ("foo *****\n");
    g_assert_true (example416 == "<p>foo *****</p>\n");

    QString example417 = parse ("foo **\\***\n");
    g_assert_true (example417 == "<p>foo <strong>*</strong></p>\n");

    QString example418 = parse ("foo **_**\n");
    g_assert_true (example418 == "<p>foo <strong>_</strong></p>\n");

    QString example419 = parse ("**foo*\n");
    g_assert_true (example419 == "<p>*<em>foo</em></p>\n");

    QString example420 = parse ("*foo**\n");
    g_assert_true (example420 == "<p><em>foo</em>*</p>\n");

    QString example421 = parse ("***foo**\n");
    g_assert_true (example421 == "<p>*<strong>foo</strong></p>\n");

    QString example422 = parse ("****foo*\n");
    g_assert_true (example422 == "<p>***<em>foo</em></p>\n");

    QString example423 = parse ("**foo***\n");
    g_assert_true (example423 == "<p><strong>foo</strong>*</p>\n");

    QString example424 = parse ("*foo****\n");
    g_assert_true (example424 == "<p><em>foo</em>***</p>\n");

    QString example425 = parse ("foo ___\n");
    g_assert_true (example425 == "<p>foo ___</p>\n");

    QString example426 = parse ("foo _\\__\n");
    g_assert_true (example426 == "<p>foo <em>_</em></p>\n");

    QString example427 = parse ("foo _*_\n");
    g_assert_true (example427 == "<p>foo <em>*</em></p>\n");

    QString example428 = parse ("foo _____\n");
    g_assert_true (example428 == "<p>foo _____</p>\n");

    QString example429 = parse ("foo __\\___\n");
    g_assert_true (example429 == "<p>foo <strong>_</strong></p>\n");

    QString example430 = parse ("foo __*__\n");
    g_assert_true (example430 == "<p>foo <strong>*</strong></p>\n");

    QString example431 = parse ("__foo_\n");
    g_assert_true (example431 == "<p>_<em>foo</em></p>\n");

    QString example432 = parse ("_foo__\n");
    g_assert_true (example432 == "<p><em>foo</em>_</p>\n");

    QString example433 = parse ("___foo__\n");
    g_assert_true (example433 == "<p>_<strong>foo</strong></p>\n");

    QString example434 = parse ("____foo_\n");
    g_assert_true (example434 == "<p>___<em>foo</em></p>\n");

    QString example435 = parse ("__foo___\n");
    g_assert_true (example435 == "<p><strong>foo</strong>_</p>\n");

    QString example436 = parse ("_foo____\n");
    g_assert_true (example436 == "<p><em>foo</em>___</p>\n");

    QString example437 = parse ("**foo**\n");
    g_assert_true (example437 == "<p><strong>foo</strong></p>\n");

    QString example438 = parse ("*_foo_*\n");
    g_assert_true (example438 == "<p><em><em>foo</em></em></p>\n");

    QString example439 = parse ("__foo__\n");
    g_assert_true (example439 == "<p><strong>foo</strong></p>\n");

    QString example440 = parse ("_*foo*_\n");
    g_assert_true (example440 == "<p><em><em>foo</em></em></p>\n");

    QString example441 = parse ("****foo****\n");
    g_assert_true (example441 == "<p><strong><strong>foo</strong></strong></p>\n");

    QString example442 = parse ("____foo____\n");
    g_assert_true (example442 == "<p><strong><strong>foo</strong></strong></p>\n");

    QString example443 = parse ("******foo******\n");
    g_assert_true (example443 == "<p><strong><strong><strong>foo</strong></strong></strong></p>\n");

    QString example444 = parse ("***foo***\n");
    g_assert_true (example444 == "<p><em><strong>foo</strong></em></p>\n");

    QString example445 = parse ("_____foo_____\n");
    g_assert_true (example445 == "<p><em><strong><strong>foo</strong></strong></em></p>\n");

    QString example446 = parse ("*foo _bar* baz_\n");
    g_assert_true (example446 == "<p><em>foo _bar</em> baz_</p>\n");

    QString example447 = parse ("*foo __bar *baz bim__ bam*\n");
    g_assert_true (example447 == "<p><em>foo <strong>bar *baz bim</strong> bam</em></p>\n");

    QString example448 = parse ("**foo **bar baz**\n");
    g_assert_true (example448 == "<p>**foo <strong>bar baz</strong></p>\n");

    QString example449 = parse ("*foo *bar baz*\n");
    g_assert_true (example449 == "<p>*foo <em>bar baz</em></p>\n");

    QString example455 = parse ("*a `*`*\n");
    g_assert_true (example455 == "<p><em>a <code>*</code></em></p>\n");

    QString example456 = parse ("_a `_`_\n");
    g_assert_true (example456 == "<p><em>a <code>_</code></em></p>\n");
}

static void
test_markdown_textual_content ()
{
    QString example622 = parse ("hello $.;'there\n");
    g_assert_true (example622 == "<p>hello $.;'there</p>\n");

    QString example623 = parse ("Foo χρῆν\n");
    g_assert_true (example623 == "<p>Foo χρῆν</p>\n");

    QString example624 = parse ("Multiple     spaces\n");
    g_assert_true (example624 == "<p>Multiple     spaces</p>\n");
}

static QString serialize_url_node (QSnapdMarkdownNode &node);

static QString
serialize_url_children (QSnapdMarkdownNode &node)
{
    QString result;
    for (int i = 0; i < node.childCount (); i++) {
        QScopedPointer<QSnapdMarkdownNode> child (node.child (i));
        result += serialize_url_node (*child);
    }
    return result;
}

static QString
serialize_url_node (QSnapdMarkdownNode &node)
{
   switch (node.type ()) {
   case QSnapdMarkdownNode::NodeTypeText:
       return escape_text (node.text ());

   case QSnapdMarkdownNode::NodeTypeParagraph:
       return "<p>" + serialize_url_children (node) + "</p>";

   case QSnapdMarkdownNode::NodeTypeUrl:
       return "<url>" + serialize_url_children (node) + "</url>";

   default:
       g_assert_not_reached ();
       return "";
   }
}

static QString
parse_url (const QString &text)
{
    QSnapdMarkdownParser parser (QSnapdMarkdownParser::MarkdownVersion0);
    QList<QSnapdMarkdownNode> nodes = parser.parse (text);
    QString result;
    for (int i = 0; i < nodes.size (); i++)
        result += serialize_url_node (nodes[i]);
    return result;
}

static void
test_markdown_urls ()
{
    QString url0 = parse_url ("http://localhost");
    g_assert_true (url0 == "<p><url>http://localhost</url></p>");

    QString url1 = parse_url ("https://localhost");
    g_assert_true (url1 == "<p><url>https://localhost</url></p>");

    QString url2 = parse_url ("mailto:name@example.com");
    g_assert_true (url2 == "<p><url>mailto:name@example.com</url></p>");

    QString url3 = parse_url ("ftp://foo");
    g_assert_true (url3 == "<p>ftp://foo</p>");

    QString url4 = parse_url ("http://");
    g_assert_true (url4 == "<p>http://</p>");

    QString url5 = parse_url ("https://");
    g_assert_true (url5 == "<p>https://</p>");

    QString url6 = parse_url ("mailto:");
    g_assert_true (url6 == "<p>mailto:</p>");

    QString url7 = parse_url (" https://localhost");
    g_assert_true (url7 == "<p><url>https://localhost</url></p>");

    QString url8 = parse_url ("https://localhost ");
    g_assert_true (url8 == "<p><url>https://localhost</url></p>");

    QString url9 = parse_url (" https://localhost ");
    g_assert_true (url9 == "<p><url>https://localhost</url></p>");

    QString url10 = parse_url ("x https://localhost");
    g_assert_true (url10 == "<p>x <url>https://localhost</url></p>");

    QString url11 = parse_url ("https://localhost x");
    g_assert_true (url11 == "<p><url>https://localhost</url> x</p>");

    QString url12 = parse_url ("x https://localhost x");
    g_assert_true (url12 == "<p>x <url>https://localhost</url> x</p>");

    QString url13 = parse_url ("(https://localhost)");
    g_assert_true (url13 == "<p>(<url>https://localhost</url>)</p>");

    QString url14 = parse_url ("https://localhost/(foo)");
    g_assert_true (url14 == "<p><url>https://localhost/(foo)</url></p>");

    QString url15 = parse_url ("https://localhost/.");
    g_assert_true (url15 == "<p><url>https://localhost/</url>.</p>");

    QString url16 = parse_url ("https://localhost/,");
    g_assert_true (url16 == "<p><url>https://localhost/</url>,</p>");
}

static QString
parse_whitespace (const QString &text)
{
    QSnapdMarkdownParser parser (QSnapdMarkdownParser::MarkdownVersion0);
    g_assert_false (parser.preserveWhitespace ());
    QList<QSnapdMarkdownNode> nodes = parser.parse (text);
    QString result;
    for (int i = 0; i < nodes.size (); i++)
        result += serialize_node (nodes[i]);
    return result;
}

static void
test_markdown_whitespace ()
{
    QString whitespace0 = parse_whitespace ("Inter  word");
    g_assert_true (whitespace0 == "<p>Inter word</p>\n");

    QString whitespace1 = parse_whitespace ("Inter    word");
    g_assert_true (whitespace1 == "<p>Inter word</p>\n");

    QString whitespace2 = parse_whitespace ("New\nline");
    g_assert_true (whitespace2 == "<p>New line</p>\n");

    QString whitespace3 = parse_whitespace ("New \n line");
    g_assert_true (whitespace3 == "<p>New line</p>\n");

    QString whitespace4 = parse_whitespace ("A  *very  emphasised*  line");
    g_assert_true (whitespace4 == "<p>A <em>very emphasised</em> line</p>\n");
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
