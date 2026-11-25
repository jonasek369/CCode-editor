#ifndef _H_QUERY_PREDICATES
#define _H_QUERY_PREDICATES

/*
    Tree-sitter leaves text predicates of highlight queries (#eq?, #match?, #any-of?)
    to the caller. Without them a pattern like
        ((identifier) @constant (#match? @constant "^[A-Z][A-Z_]*$"))
    would color every identifier.

    The regex engine is a small backtracking matcher (Windows has no regex.h) supporting
    literals, '.', [classes], [^negated], \d \w \s \D \W \S, escapes, groups with '|',
    the quantifiers * + ? and the anchors ^ $.
*/

#define RX_MAX_DEPTH 2000

typedef struct {
    const char* p;
    const char* end;
} RxSeg;

typedef struct {
    const char* text_start;
    const char* text_end;
    int depth;
} RxState;

static bool rx_match_segs(RxState* st, RxSeg* segs, int nsegs, const char* t);

// End of the atom starting at p
static const char* rx_atom_end(const char* p, const char* end){
    if(p >= end) return p;
    if(*p == '\\') return p + 2 <= end ? p + 2 : end;
    if(*p == '['){
        const char* q = p + 1;
        if(q < end && *q == '^') q++;
        if(q < end && *q == ']') q++;
        while(q < end && *q != ']'){
            if(*q == '\\') q++;
            q++;
        }
        return q < end ? q + 1 : end;
    }
    if(*p == '('){
        int depth = 0;
        const char* q = p;
        while(q < end){
            if(*q == '\\'){ q += 2; continue; }
            if(*q == '[') { q = rx_atom_end(q, end); continue; }
            if(*q == '(') depth++;
            else if(*q == ')'){ depth--; if(depth == 0) return q + 1; }
            q++;
        }
        return end;
    }
    return p + 1;
}

static bool rx_class_escape(char e, unsigned char c){
    switch(e){
        case 'd': return isdigit(c);
        case 'D': return !isdigit(c);
        case 'w': return isalnum(c) || c == '_';
        case 'W': return !(isalnum(c) || c == '_');
        case 's': return isspace(c);
        case 'S': return !isspace(c);
        default:  return false;
    }
}

static bool rx_is_class_escape(char e){
    return e == 'd' || e == 'D' || e == 'w' || e == 'W' || e == 's' || e == 'S';
}

static char rx_escape_char(char e){
    switch(e){
        case 'n': return '\n';
        case 't': return '\t';
        case 'r': return '\r';
        default:  return e;
    }
}

// Does the single character atom [p, aend) match c
static bool rx_atom_matches(const char* p, const char* aend, unsigned char c){
    if(*p == '.') return c != '\n';
    if(*p == '\\'){
        if(rx_is_class_escape(p[1])) return rx_class_escape(p[1], c);
        return c == (unsigned char)rx_escape_char(p[1]);
    }
    if(*p == '['){
        const char* q = p + 1;
        bool negate = false;
        if(*q == '^'){ negate = true; q++; }
        const char* cend = aend - 1; // the closing ']'
        bool found = false;
        bool first = true;
        while(q < cend){
            unsigned char lo;
            if(*q == '\\'){
                if(rx_is_class_escape(q[1])){
                    if(rx_class_escape(q[1], c)) found = true;
                    q += 2;
                    first = false;
                    continue;
                }
                lo = (unsigned char)rx_escape_char(q[1]);
                q += 2;
            }else{
                lo = (unsigned char)*q;
                q++;
            }
            (void) first;
            first = false;
            if(q + 1 < cend && *q == '-'){
                unsigned char hi;
                if(q[1] == '\\'){ hi = (unsigned char)rx_escape_char(q[2]); q += 3; }
                else { hi = (unsigned char)q[1]; q += 2; }
                if(c >= lo && c <= hi) found = true;
            }else if(c == lo){
                found = true;
            }
        }
        return negate ? !found : found;
    }
    return c == (unsigned char)*p;
}

// Matches a group body [p, end) with alternation, then the continuation
static bool rx_match_alternatives(RxState* st, const char* p, const char* end, RxSeg* rest, int nrest, const char* t){
    const char* alt = p;
    const char* q = p;
    int depth = 0;
    RxSeg segs[64];
    if(nrest + 1 > 64) return false;
    for(;;){
        if(q >= end || (*q == '|' && depth == 0)){
            segs[0].p = alt;
            segs[0].end = q;
            memcpy(segs + 1, rest, nrest * sizeof(RxSeg));
            if(rx_match_segs(st, segs, nrest + 1, t)) return true;
            if(q >= end) return false;
            alt = q + 1;
            q++;
            continue;
        }
        if(*q == '\\'){ q += 2; continue; }
        if(*q == '['){ q = rx_atom_end(q, end); continue; }
        if(*q == '(') depth++;
        if(*q == ')') depth--;
        q++;
    }
}

static bool rx_match_segs(RxState* st, RxSeg* segs, int nsegs, const char* t){
    if(++st->depth > RX_MAX_DEPTH){ st->depth--; return false; }
    bool result = false;

    // drop finished segments
    while(nsegs > 0 && segs[0].p >= segs[0].end){ segs++; nsegs--; }
    if(nsegs == 0){ result = true; goto done; }

    const char* p = segs[0].p;
    const char* end = segs[0].end;

    // top level alternation of this segment
    {
        int depth = 0;
        for(const char* q = p; q < end; ){
            if(*q == '\\'){ q += 2; continue; }
            if(*q == '['){ q = rx_atom_end(q, end); continue; }
            if(*q == '(') depth++;
            else if(*q == ')') depth--;
            else if(*q == '|' && depth == 0){
                result = rx_match_alternatives(st, p, end, segs + 1, nsegs - 1, t);
                goto done;
            }
            q++;
        }
    }

    if(*p == '^'){
        if(t != st->text_start){ result = false; goto done; }
        RxSeg copy[64];
        memcpy(copy, segs, nsegs * sizeof(RxSeg));
        copy[0].p = p + 1;
        result = rx_match_segs(st, copy, nsegs, t);
        goto done;
    }
    if(*p == '$' && p + 1 == end){
        RxSeg copy[64];
        memcpy(copy, segs, nsegs * sizeof(RxSeg));
        copy[0].p = p + 1;
        result = t == st->text_end && rx_match_segs(st, copy, nsegs, t);
        goto done;
    }

    {
        const char* aend = rx_atom_end(p, end);
        char quant = (aend < end && (*aend == '*' || *aend == '+' || *aend == '?')) ? *aend : 0;
        const char* after = quant ? aend + 1 : aend;
        // lazy quantifiers are matched greedily
        if(quant && after < end && *after == '?') after++;

        RxSeg rest[64];
        if(nsegs > 63){ result = false; goto done; }
        memcpy(rest, segs, nsegs * sizeof(RxSeg));
        rest[0].p = after;

        if(*p == '('){
            const char* body = p + 1;
            const char* body_end = aend - 1;
            // non capturing groups
            if(body + 1 < body_end && body[0] == '?' && body[1] == ':') body += 2;
            if(quant == 0){
                result = rx_match_alternatives(st, body, body_end, rest, nsegs, t);
            }else if(quant == '?'){
                result = rx_match_alternatives(st, body, body_end, rest, nsegs, t) || rx_match_segs(st, rest, nsegs, t);
            }else{
                // again = the group with its quantifier followed by the rest:
                //   (X)* rest = X (X)* rest | rest
                //   (X)+ rest = X (X)+ rest | X rest
                // an X matching nothing recurses on the same position until RX_MAX_DEPTH
                RxSeg again[65];
                again[0].p = p;
                again[0].end = after;
                memcpy(again + 1, rest, nsegs * sizeof(RxSeg));
                result = t < st->text_end && rx_match_alternatives(st, body, body_end, again, nsegs + 1, t);
                if(!result){
                    result = quant == '*' ? rx_match_segs(st, rest, nsegs, t)
                                          : rx_match_alternatives(st, body, body_end, rest, nsegs, t);
                }
            }
            goto done;
        }

        if(quant == 0){
            result = t < st->text_end && rx_atom_matches(p, aend, (unsigned char)*t) && rx_match_segs(st, rest, nsegs, t + 1);
            goto done;
        }
        if(quant == '?'){
            result = (t < st->text_end && rx_atom_matches(p, aend, (unsigned char)*t) && rx_match_segs(st, rest, nsegs, t + 1)) ||
                     rx_match_segs(st, rest, nsegs, t);
            goto done;
        }
        // * and +, greedy with backtracking
        const char* q = t;
        while(q < st->text_end && rx_atom_matches(p, aend, (unsigned char)*q)) q++;
        const char* min = quant == '+' ? t + 1 : t;
        for(; q >= min; q--){
            if(rx_match_segs(st, rest, nsegs, q)){ result = true; goto done; }
            if(q == t) break;
        }
        result = false;
    }

done:
    st->depth--;
    return result;
}

// Searches pattern anywhere in text (like tree-sitter's #match?)
bool rx_search(const char* pattern, const char* text, size_t len){
    RxState st = { .text_start = text, .text_end = text + len, .depth = 0 };
    RxSeg seg = { pattern, pattern + strlen(pattern) };
    bool anchored = pattern[0] == '^';
    for(const char* t = text; t <= text + len; t++){
        RxSeg s = seg;
        st.depth = 0;
        if(rx_match_segs(&st, &s, 1, t)) return true;
        if(anchored) break;
    }
    return false;
}


/*
    Predicates
*/

static char* predicate_node_text(LayerCodeData* lcd, TSNode node, size_t* len){
    TSPoint s = ts_node_start_point(node);
    TSPoint e = ts_node_end_point(node);
    if((int)s.row >= arrlen(lcd->code_buffer)){
        *len = 0;
        return strdup("");
    }
    if((int)e.row >= arrlen(lcd->code_buffer)){
        e.row = arrlen(lcd->code_buffer) - 1;
        e.column = (uint32_t)line_content_length(lcd->code_buffer[e.row]);
    }
    return layer_code_get_text(lcd, s.row, s.column, e.row, e.column, len);
}

static bool match_capture_node(const TSQueryMatch* match, uint32_t capture_id, TSNode* out){
    for(uint16_t i = 0; i < match->capture_count; i++){
        if(match->captures[i].index == capture_id){
            *out = match->captures[i].node;
            return true;
        }
    }
    return false;
}

// True when every text predicate of the match's pattern holds
bool query_match_passes(LayerCodeData* lcd, const TSQuery* query, const TSQueryMatch* match){
    uint32_t step_count = 0;
    const TSQueryPredicateStep* steps = ts_query_predicates_for_pattern(query, match->pattern_index, &step_count);
    if(step_count == 0) return true;

    uint32_t i = 0;
    while(i < step_count){
        // one predicate: name, args..., Done
        uint32_t start = i;
        while(i < step_count && steps[i].type != TSQueryPredicateStepTypeDone) i++;
        uint32_t end = i; // exclusive
        i++;
        if(end - start < 2 || steps[start].type != TSQueryPredicateStepTypeString) continue;

        uint32_t name_len;
        const char* name = ts_query_string_value_for_id(query, steps[start].value_id, &name_len);
        bool negate = strncmp(name, "not-", 4) == 0;
        const char* op = negate ? name + 4 : name;

        if(steps[start + 1].type != TSQueryPredicateStepTypeCapture) continue;
        TSNode node;
        if(!match_capture_node(match, steps[start + 1].value_id, &node)) continue;
        size_t text_len;
        char* text = predicate_node_text(lcd, node, &text_len);

        bool ok = true;
        if(strcmp(op, "eq?") == 0 && end - start >= 3){
            if(steps[start + 2].type == TSQueryPredicateStepTypeString){
                uint32_t vl;
                const char* v = ts_query_string_value_for_id(query, steps[start + 2].value_id, &vl);
                ok = vl == text_len && memcmp(v, text, vl) == 0;
            }else{
                TSNode other;
                if(match_capture_node(match, steps[start + 2].value_id, &other)){
                    size_t ol;
                    char* ot = predicate_node_text(lcd, other, &ol);
                    ok = ol == text_len && memcmp(ot, text, ol) == 0;
                    free(ot);
                }
            }
        }else if(strcmp(op, "match?") == 0 && end - start >= 3 && steps[start + 2].type == TSQueryPredicateStepTypeString){
            uint32_t vl;
            const char* v = ts_query_string_value_for_id(query, steps[start + 2].value_id, &vl);
            ok = rx_search(v, text, text_len);
        }else if(strcmp(op, "any-of?") == 0){
            ok = false;
            for(uint32_t k = start + 2; k < end; k++){
                if(steps[k].type != TSQueryPredicateStepTypeString) continue;
                uint32_t vl;
                const char* v = ts_query_string_value_for_id(query, steps[k].value_id, &vl);
                if(vl == text_len && memcmp(v, text, vl) == 0){ ok = true; break; }
            }
        }else{
            // unknown predicates (#set!, #is?, ...) don't filter
            free(text);
            continue;
        }
        free(text);
        if(negate) ok = !ok;
        if(!ok) return false;
    }
    return true;
}

#endif // _H_QUERY_PREDICATES
