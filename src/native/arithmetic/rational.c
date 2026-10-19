#include <flint/fmpq.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static int all_digits(const char *s, size_t n) {
    if (n == 0) return 0;
    for (size_t i = 0; i < n; ++i) if (!isdigit((unsigned char)s[i])) return 0;
    return 1;
}

int htb_parse_fmpq(fmpq_t result, const char *text, char *error, size_t error_size) {
    if (!text || !*text || strlen(text) > 150) {
        snprintf(error, error_size, "scalar must be a nonempty string of at most 150 characters");
        return 0;
    }
    const char *s = text;
    int negative = 0;
    if (*s == '-' || *s == '+') { negative = *s == '-'; s++; }
    if (!*s || strchr(s, ' ') || strchr(s, '\t') || strchr(s, '\n')) {
        snprintf(error, error_size, "invalid rational: %s", text);
        return 0;
    }
    const char *slash = strchr(s, '/');
    const char *dot = strchr(s, '.');
    fmpz_t num, den;
    fmpz_init(num); fmpz_init(den);
    int ok = 0;
    if (slash) {
        if (dot || strchr(slash + 1, '/') || !all_digits(s, (size_t)(slash - s)) ||
            !all_digits(slash + 1, strlen(slash + 1))) goto invalid;
        char *n = strndup(s, (size_t)(slash - s));
        char *d = strdup(slash + 1);
        if (!n || !d) { free(n); free(d); goto invalid; }
        ok = fmpz_set_str(num, n, 10) == 0 && fmpz_set_str(den, d, 10) == 0 && !fmpz_is_zero(den);
        free(n); free(d);
    } else if (dot) {
        if (strchr(dot + 1, '.') || (dot != s && !all_digits(s, (size_t)(dot - s))) ||
            !all_digits(dot + 1, strlen(dot + 1)) || dot == s) goto invalid;
        size_t before = (size_t)(dot - s), after = strlen(dot + 1);
        char *digits = malloc(before + after + 1);
        if (!digits) goto invalid;
        memcpy(digits, s, before);
        memcpy(digits + before, dot + 1, after + 1);
        ok = fmpz_set_str(num, digits, 10) == 0;
        fmpz_set_ui(den, 10);
        fmpz_pow_ui(den, den, after);
        free(digits);
    } else {
        if (!all_digits(s, strlen(s))) goto invalid;
        ok = fmpz_set_str(num, s, 10) == 0;
        fmpz_one(den);
    }
    if (!ok) goto invalid;
    if (negative) fmpz_neg(num, num);
    fmpz_set(fmpq_numref(result), num);
    fmpz_set(fmpq_denref(result), den);
    fmpq_canonicalise(result);
    fmpz_clear(num); fmpz_clear(den);
    return 1;
invalid:
    snprintf(error, error_size, "invalid exact rational: %s", text);
    fmpz_clear(num); fmpz_clear(den);
    return 0;
}

