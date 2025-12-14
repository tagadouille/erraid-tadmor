
/* --------------------- string encoding ---------------------
   Wire format: LENGTH (uint32) followed by LENGTH bytes (no terminal 0).
   Here we assume string_t { uint32_t length; char *data; }.
-----------------------------------------------------------------*/

int encode_string(int fd, const string_t *s) {
    if (!s) return -1;
    if (s->length > LIMIT_MAX_STR_LEN) return -1;
    if (encode_uint32(fd, s->length) < 0) return -1;
    if (s->length > 0) {
        if (write_full(fd, s->data, s->length) < 0) return -1;
    }
    return 0;
}

int decode_string(int fd, string_t *s) {
    if (!s) return -1;
    if (decode_uint32(fd, &s->length) < 0) return -1;
    if (s->length > LIMIT_MAX_STR_LEN) return -1;
    if (s->length > 0) {
        s->data = malloc((size_t)s->length + 1);
        if (!s->data) return -1;
        if (read_full(fd, s->data, s->length) < 0) {
            free(s->data);
            return -1;
        }
        s->data[s->length] = '\0';
    } else {
        s->data = NULL;
    }
    return 0;
}

/* --------------------- timing encoding --------------------- */
/* timing: MINUTES(uint64), HOURS(uint32), DAYSOFWEEK(uint8) */

int encode_timing(int fd, const timing_t *t) {
    if (!t) return -1;
    if (encode_uint64(fd, t->minutes) < 0) return -1;
    if (encode_uint32(fd, t->hours) < 0) return -1;
    if (encode_uint8(fd, t->daysofweek) < 0) return -1;
    return 0;
}
int decode_timing(int fd, timing_t *t) {
    if (!t) return -1;
    if (decode_uint64(fd, &t->minutes) < 0) return -1;
    if (decode_uint32(fd, &t->hours) < 0) return -1;
    if (decode_uint8(fd, &t->daysofweek) < 0) return -1;
    return 0;
}

/* --------------------- arguments encoding ---------------------
 * Protocol: ARGC (uint32), then ARGV[0..ARGC-1] as strings.
 * Note: for CREATE request the COMMAND is exactly this arguments_t per protocol.
 * In your in-memory arguments_t we assume fields: uint32_t argc; string_t **argv;
 * (no separate command field). If your structure differs, adapt accordingly.
 -----------------------------------------------------------------*/

int encode_arguments(int fd, const arguments_t *args) {
    if (!args) return -1;
    if (args->argc == 0 || args->argc > LIMIT_MAX_ARGC) return -1;
    if (encode_uint32(fd, args->argc) < 0) return -1;
    for (uint32_t i = 0; i < args->argc; ++i) {
        if (encode_string(fd, args->argv[i]) < 0) return -1;
    }
    return 0;
}

int decode_arguments(int fd, arguments_t *args) {
    if (!args) return -1;
    uint32_t argc;
    if (decode_uint32(fd, &argc) < 0) return -1;
    if (argc == 0 || argc > LIMIT_MAX_ARGC) return -1;
    args->argc = argc;
    args->argv = calloc((size_t)argc, sizeof(string_t*));
    if (!args->argv) return -1;
    for (uint32_t i = 0; i < argc; ++i) {
        args->argv[i] = malloc(sizeof(string_t));
        if (!args->argv[i]) {
            /* cleanup */
            for (uint32_t j = 0; j < i; ++j) {
                free(args->argv[j]->data);
                free(args->argv[j]);
            }
            free(args->argv);
            return -1;
        }
        if (decode_string(fd, args->argv[i]) < 0) {
            /* cleanup */
            for (uint32_t j = 0; j <= i; ++j) {
                if (args->argv[j]) { free(args->argv[j]->data); free(args->argv[j]); }
            }
            free(args->argv);
            return -1;
        }
    }
    return 0;
}

/* --------------------- helper: build commandline string ----------
 * Protocol LIST wants TASK.N.COMMANDLINE as a single string.
 * If command is simple (SI) we join argv with spaces.
 * If command is composed (SQ) we flatten subcommands separated by " | " (or " ; ").
 * We return a freshly malloced string_t (filled) or NULL on error.
 * Caller must free->data and struct.
 -----------------------------------------------------------------*/
static string_t *command_to_commandline(const command_t *cmd)
{
    if (!cmd) return NULL;
    /* If simple: join argv strings */
    if (cmd->type == SI) {
        /* compute length */
        size_t total = 0;
        uint32_t argc = cmd->args.simple->argc;
        if (argc == 0) return NULL;
        for (uint32_t i = 0; i < argc; ++i) {
            string_t *s = cmd->args.simple->argv[i];
            if (!s) return NULL;
            total += s->length;
            if (i + 1 < argc) total += 1; /* space */
        }
        if (total > LIMIT_MAX_STR_LEN) return NULL;
        string_t *out = malloc(sizeof(string_t));
        if (!out) return NULL;
        out->length = (uint32_t)total;
        out->data = malloc(total + 1);
        if (!out->data) { free(out); return NULL; }
        size_t off = 0;
        for (uint32_t i = 0; i < argc; ++i) {
            string_t *s = cmd->args.simple->argv[i];
            memcpy(out->data + off, s->data, s->length);
            off += s->length;
            if (i + 1 < argc) {
                out->data[off++] = ' ';
            }
        }
        out->data[off] = '\0';
        return out;
    } else {
        /* composed: build by concatenating subcommands with ' ; ' */
        size_t total = 0;
        uint16_t n = cmd->args.composed.count;
        if (n == 0) return NULL;
        string_t **subs = calloc(n, sizeof(string_t*));
        if (!subs) return NULL;
        for (uint16_t i = 0; i < n; ++i) {
            subs[i] = command_to_commandline(cmd->args.composed.cmds[i]);
            if (!subs[i]) {
                for (uint16_t j = 0; j < i; ++j) {
                    free(subs[j]->data);
                    free(subs[j]);
                }
                free(subs);
                return NULL;
            }
            total += subs[i]->length;
            if (i + 1 < n) total += 3; /* " ; " */
        }
        if (total > LIMIT_MAX_STR_LEN) {
            for (uint16_t i = 0; i < n; ++i) { free(subs[i]->data); free(subs[i]); }
            free(subs);
            return NULL;
        }
        string_t *out = malloc(sizeof(string_t));
        if (!out) { for (uint16_t i = 0; i < n; ++i) { free(subs[i]->data); free(subs[i]); } free(subs); return NULL; }
        out->length = (uint32_t)total;
        out->data = malloc(total + 1);
        if (!out->data) { free(out); for (uint16_t i = 0; i < n; ++i) { free(subs[i]->data); free(subs[i]); } free(subs); return NULL; }
        size_t off = 0;
        for (uint16_t i = 0; i < n; ++i) {
            memcpy(out->data + off, subs[i]->data, subs[i]->length);
            off += subs[i]->length;
            if (i + 1 < n) {
                out->data[off++] = ' ';
                out->data[off++] = ';';
                out->data[off++] = ' ';
            }
            free(subs[i]->data);
            free(subs[i]);
        }
        free(subs);
        out->data[off] = '\0';
        return out;
    }
}