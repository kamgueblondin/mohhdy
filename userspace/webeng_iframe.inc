/* Lot 74: iframe srcdoc= embeds a nested HTML fragment as children of the
 * iframe. No network src= fetch. The fragment shares the document node
 * table. Nested iframe srcdoc is refused. attr_flags bit 64 marks srcdoc. */
#define WE_ATTR_SRCDOC 64

static int iframe_parse_fragment(we_doc_t *d, int parent, const char *html, int len);

/* called from the attribute loop for an iframe element */
static int iframe_attr(we_doc_t *d, int idx, const char *an, const char *av, int vk) {
    we_node_t *n = &d->nodes[idx];
    if (w_eq(an, "srcdoc")) {
        if (pool_put(d, av, vk, &n->src_off) == 0) {
            n->src_len = (uint16_t)vk;
            n->attr_flags |= WE_ATTR_SRCDOC;
        }
        return 1;
    }
    if (w_eq(an, "src")) {
        if (!(n->attr_flags & WE_ATTR_SRCDOC)) diag(d, "iframe src refused", idx);
        return 1;
    }
    return 0;
}

static void iframe_expand(we_doc_t *d) {
    int i, n0 = (int)d->nnodes;
    for (i = 1; i < n0; i++) {
        we_node_t *n = &d->nodes[i];
        if (!w_eq(n->tag, "iframe") || !(n->attr_flags & WE_ATTR_SRCDOC) || !n->src_len) continue;
        if (n->first >= 0) continue;
        if (iframe_parse_fragment(d, i, d->pool + n->src_off, n->src_len) != 0)
            diag(d, "iframe srcdoc refused", i);
    }
}

static int iframe_parse_fragment(we_doc_t *d, int parent, const char *html, int len) {
    int stack[WE_DEPTH_MAX], sp = 0, i = 0, text_start = 0;
    stack[sp++] = parent;
    while (i <= len) {
        if (i == len || html[i] == '<') {
            if (i > text_start) {
                if (add_text(d, stack[sp - 1], html + text_start, i - text_start) != 0) {
                    diag(d, "iframe srcdoc nodes", parent);
                    return -1;
                }
            }
            if (i == len) break;
            if (w_ieq_n(html + i, len - i, "<!--")) {
                int e = find_ci(html, len, i + 4, "-->");
                i = e < 0 ? len : e + 3;
                text_start = i;
                continue;
            }
            if (i + 1 < len && html[i + 1] == '/') {
                char name[12];
                int k = 0, j = i + 2, s;
                while (j < len && w_namec(html[j]) && k < 11) name[k++] = w_low(html[j++]);
                name[k] = 0;
                while (j < len && html[j] != '>') j++;
                i = j + 1; text_start = i;
                for (s = sp - 1; s > 0; s--)
                    if (w_eq(d->nodes[stack[s]].tag, name)) { sp = s; break; }
                continue;
            }
            if (i + 1 < len && (html[i + 1] == '!' || html[i + 1] == '?')) {
                while (i < len && html[i] != '>') i++;
                i++; text_start = i; continue;
            }
            {
                char name[12];
                int k = 0, j = i + 1, idx, self = 0;
                we_node_t *n;
                while (j < len && w_namec(html[j]) && k < 11) name[k++] = w_low(html[j++]);
                name[k] = 0;
                if (!k) { i++; text_start = i; continue; }
                if (w_eq(name, "iframe")) {
                    diag(d, "iframe nested srcdoc refused", parent);
                    while (j < len && html[j] != '>') j++;
                    i = j + 1; text_start = i; continue;
                }
                idx = new_node(d, stack[sp - 1]);
                if (idx < 0) { diag(d, "iframe srcdoc nodes", parent); return -1; }
                n = &d->nodes[idx];
                w_cpy(n->tag, 12, name, k);
                while (j < len && html[j] != '>') {
                    char an[16]; int ak = 0, vk = 0; const char *av = html + j;
                    while (j < len && w_space(html[j])) j++;
                    if (j >= len || html[j] == '>') break;
                    if (html[j] == '/') { self = 1; j++; continue; }
                    ak = 0;
                    while (j < len && w_namec(html[j]) && ak < 15) an[ak++] = w_low(html[j++]);
                    an[ak] = 0;
                    while (j < len && w_space(html[j])) j++;
                    vk = 0; av = html + j;
                    if (j < len && html[j] == '=') {
                        char q;
                        j++; while (j < len && w_space(html[j])) j++;
                        if (j < len && (html[j] == '"' || html[j] == '\'')) {
                            q = html[j++]; av = html + j;
                            while (j < len && html[j] != q) j++;
                            vk = (int)(html + j - av); if (j < len) j++;
                        } else {
                            av = html + j;
                            while (j < len && !w_space(html[j]) && html[j] != '>') j++;
                            vk = (int)(html + j - av);
                        }
                    }
                    if (w_eq(an, "id") && vk < (int)sizeof(n->id)) {
                        w_cpy(n->id, 16, av, vk); n->attr_flags |= 1;
                    } else if (w_eq(an, "class")) {
                        w_cpy(n->cls, 32, av, vk); n->attr_flags |= 2;
                    } else if (w_eq(an, "style")) {
                        if (pool_put(d, av, vk, &n->style_off) == 0) n->style_len = (uint16_t)vk;
                    } else if (forms_tag(name)) forms_attr(d, idx, an, av, vk);
                }
                if (j < len && html[j] == '>') j++;
                i = j; text_start = i;
                if (w_eq(name, "style")) {
                    int e = find_ci(html, len, i, "</style");
                    int end = e < 0 ? len : e, c;
                    for (c = i; c < end && d->css_len < WE_CSS_MAX - 1; c++) d->css[d->css_len++] = html[c];
                    if (d->css_len < WE_CSS_MAX) d->css[d->css_len++] = ' ';
                    if (e < 0) i = len; else { while (e < len && html[e] != '>') e++; i = e + 1; }
                    text_start = i;
                    continue;
                }
                if (!self && !is_void(name)) {
                    if (sp >= WE_DEPTH_MAX) { diag(d, "iframe srcdoc depth", i); return -1; }
                    stack[sp++] = idx;
                }
            }
            continue;
        }
        i++;
    }
    return 0;
}
