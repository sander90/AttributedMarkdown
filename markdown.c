/**********************************************************************

  markdown.c - markdown in C using a PEG grammar. 
  (c) 2008 John MacFarlane (jgm at berkeley dot edu).

  This program is free software; you can redistribute it and/or modify
  it under the terms of the GNU General Public License as published by
  the Free Software Foundation; version 2 of the License.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

 ***********************************************************************/

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <glib.h>
#include "markdown_peg.h"

static int extensions;

/**********************************************************************

  Main program and auxiliary functions.

  Reads input from files on command line, or from stdin
  if no arguments given.  Converts tabs to spaces using TABSTOP.
  Parses the result for references (References), and then again for
  conversion to HTML (Doc).  The parser actions print the converted
  HTML, so there is no separate printing step here.  Character encodings
  are ignored.

 ***********************************************************************/

#define VERSION "0.2.3"
#define COPYRIGHT "Copyright (c) 2008 John MacFarlane.\n" \
                  "License GPLv2+: GNU GPL version 2 or later <http://gnu.org/licenses/gpl.html>\n" \
                  "This is free software: you are free to change and redistribute it.\n" \
                  "There is NO WARRANTY, to the extent permitted by law."

/* print version and copyright information */
void version(char *progname)
{
  printf("%s version %s\n"
         "%s\n",
         progname,
         VERSION,
         COPYRIGHT);
}

int main(int argc, char * argv[]) {

    int numargs;            /* number of filename arguments */
    FILE *inputs[argc];     /* array of file pointers for inputs */
    int lastinp;            /* index of last file in inputs */
    int i;

    GString *inputbuf;

    FILE *input;
    FILE *output;
    char curchar;
    char *progname = argv[0];
    /* the output filename is initially 0 (a.k.a. stdout) */

    int output_format = HTML_FORMAT;

    static gboolean opt_version = FALSE;
    static gchar *opt_output = 0;
    static gchar *opt_to = 0;
    static gboolean opt_smart = FALSE;
    static gboolean opt_notes = FALSE;
    static gboolean opt_allext = FALSE;

    static GOptionEntry entries[] =
    {
      { "version", 'v', 0, G_OPTION_ARG_NONE, &opt_version, "print version and exit", NULL },
      { "output", 'o', 0, G_OPTION_ARG_STRING, &opt_output, "send output to FILE (default is stdout)", "FILE" },
      { "to", 't', 0, G_OPTION_ARG_STRING, &opt_to, "convert to FORMAT (default is html)", "FORMAT" },
      { "extensions", 'x', 0, G_OPTION_ARG_NONE, &opt_allext, "use all syntax extensions", NULL },
      { NULL }
    };

    static GOptionEntry ext_entries[] =
    {
      { "smart", 0, 0, G_OPTION_ARG_NONE, &opt_smart, "use smart typography extension", NULL },
      { "notes", 0, 0, G_OPTION_ARG_NONE, &opt_notes, "use notes extension", NULL },
      { NULL }
    };

    GError *error = NULL;
    GOptionContext *context;
    GOptionGroup *ext_group;

    context = g_option_context_new ("[FILE...]");
    g_option_context_add_main_entries (context, entries, NULL);
    ext_group = g_option_group_new ("extensions", "Syntax extensions", "show available syntax extensions", NULL, NULL);
    g_option_group_add_entries (ext_group, ext_entries);
    g_option_context_add_group (context, ext_group);
    g_option_context_set_description (context, "Converts text in specified files (or stdin) from markdown to FORMAT.\n"
                                               "Available FORMATs:  html, latex, groff-mm");
    if (!g_option_context_parse (context, &argc, &argv, &error)) {
        g_print ("option parsing failed: %s\n", error->message);
        exit (1);
    }

    if (opt_version) {
        version(progname);
        return EXIT_SUCCESS;
    }

    extensions = 0;
    if (opt_allext)
        extensions = 0xFFFFFF;  /* turn on all extensions */
    if (opt_smart)
        extensions = extensions | EXT_SMART;
    if (opt_notes)
        extensions = extensions | EXT_NOTES;

    if (opt_to == NULL)
        output_format = HTML_FORMAT;
    else if (strcmp(opt_to, "html") == 0)
        output_format = HTML_FORMAT;
    else if (strcmp(opt_to, "latex") == 0)
        output_format = LATEX_FORMAT;
    else if (strcmp(opt_to, "groff-mm") == 0)
        output_format = GROFF_MM_FORMAT;
    else {
        fprintf(stderr, "%s: Unknown output format '%s'\n", progname, opt_to);
        exit(EXIT_FAILURE);
    }

    /* we allow "-" as a synonym for stdout here */
    if (opt_output == NULL || strcmp(opt_output, "-") == 0)
        output = stdout;
    else if (!(output = fopen(opt_output, "w"))) {
        perror(opt_output);
        return 1;
    }
        
    numargs = argc - 1;
    if (numargs == 0) {        /* use stdin if no files specified */
       inputs[0] = stdin;
       lastinp = 0;
    }
    else {                  /* open all the files on command line */
       for (i = 0; i < numargs; i++)
            if ((inputs[i] = fopen(argv[i+1], "r")) == NULL) {
                perror(argv[i+1]);
                exit(EXIT_FAILURE);
            }
       lastinp = i - 1;
    }

    inputbuf = g_string_new("");
   
    for (i=0; i <= lastinp; i++) {
        input = inputs[i];
        while ((curchar = fgetc(input)) != EOF)
            g_string_append_c(inputbuf, curchar);
        fclose(input);
    }

    markdown_to_stream(inputbuf->str, extensions, output_format, output);
    fprintf(output, "\n");
    g_string_free(inputbuf, true);
    g_option_context_free(context);

    return(EXIT_SUCCESS);
}

