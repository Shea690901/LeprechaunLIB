static string normalize_whitespace(string str)
{
    if (sscanf(str, "%*s\n")) {
        str = implode(explode(str, "\n"), "");
    }
    if (sscanf(str, "%*s\r")) {
        str = implode(explode(str, "\r"), "");
    }
    if (sscanf(str, "%*s\t")) {
        str = implode(explode(str, "\t"), "");
    }
    if (sscanf(str, "%*s ")) {
        str = implode(explode(str, " ") - ({ "" }), " ");
    }
    return str;
}

static string replace_string(string str, string from, string to)
{
    if (sscanf(from, "%*s%%")) {
        from = implode(explode("%" + from + "%", "%"), "%%");
    }
    if (sscanf(str, "%*s" + from)) {
        str = implode(explode(from + str + from, from), to);
    }
    return str;
}

static int has_suffix(string str, string suffix)
{
    int len, suffix_len;

    len = strlen(str);
    suffix_len = strlen(suffix);
    return len >= suffix_len && str[len - suffix_len ..] == suffix;
}

static string replace_suffix(string str, string from, string to)
{
    if (has_suffix(str, from)) {
        return str[.. strlen(str) - strlen(from) - 1] + to;
    } else {
        return str;
    }
}

static string remove_suffix(string str, string suffix)
{
    if (has_suffix(str, suffix)) {
        return str[.. strlen(str) - strlen(suffix) - 1];
    } else {
        return str;
    }
}

static string add_suffix(string str, string suffix)
{
    return has_suffix(str, suffix) ? str : str + suffix;
}