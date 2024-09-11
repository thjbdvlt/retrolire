/* add_entries
 * -----------
 *  
 * add entries to the database.
 *
 * */

// from a bibtex file
int
command_add_bibtex(char* filepath, int remove_file);

// from a DOI
int
command_add_doi(char* doi);

// from an ISBN
int
command_add_isbn(char* isbn);

// from a CSL-JSON file
int
command_add_json(char* filepath, int remove_file);

// from a template, edited in $EDITOR
int
command_add_template(char* template_name);

// format a bibtex file with pandoc, through a conversion
// --from bibtex --to bibtex
int
format_bibtex(char* filepath);
