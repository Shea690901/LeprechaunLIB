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

static int has_substring(string str, string sub)
{
    if (sscanf(sub, "%*s%%")) {
        sub = implode(explode("%" + sub + "%", "%"), "%%");
    }
    return sscanf(str, "%*s" + sub);
}

static string replace_string(string str, string from, string to)
{
    if (has_substring(str, from)) {
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

static string capitalize(string str)
{
    if (strlen(str) && str[0] >= 'a' && str[0] <= 'z') {
        str[0] = str[0] - 'a' + 'A';
    }
    return str;
}
