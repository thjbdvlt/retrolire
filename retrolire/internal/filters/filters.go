// Package filters - Build SQL filtering clause from positional arguments
package filters

import (
	"strings"

	"retrolire/internal/config"
	"retrolire/internal/opts"
)

// filter - Transform a cli argument into SQL clause and parameters
type filter = func(string, *opts.Opts) (string, []any)

func filterVar(s string, _ *opts.Opts) (clause string, params []any) {
	const sep = config.FilterFieldValueSep
	const valueSep = config.FilterValuesSep
	idx := strings.Index(s, sep)
	if idx > 0 {
		key, value := s[:idx], s[idx+1:]
		aliasedKey, ok := config.AliasesCSL[key]
		if ok {
			key = aliasedKey
		}
		values := strings.Split(value, valueSep)
		var clauses []string
		for _, v := range values {
			clauses = append(clauses, "e.csl ->> ? LIKE ?")
			params = append(params, key, "%"+v+"%")
		}
		clause = strings.Join(clauses, " OR ")
		clause = "(" + clause + ")"
	}
	return clause, params
}

func filterAuthor(s string, _ *opts.Opts) (clause string, params []any) {
	const pfx = config.FilterAuthorPrefix
	const lp = len(pfx)
	idx := strings.Index(s, pfx)
	if idx == 0 {
		clause = "lower(e.csl ->> 'author') LIKE ?"
		params = []any{"%" + s[lp:] + "%"}
	}
	return clause, params
}

func filterTag(s string, _ *opts.Opts) (clause string, params []any) {
	const tagPrefixLen = len(config.FilterTagPrefix)
	idx := strings.Index(s, config.FilterTagPrefix)
	if idx == 0 {
		clause = `EXISTS (SELECT 1 FROM tag WHERE entry = e.id AND tag = ?)`
		p := s[tagPrefixLen:]
		params = []any{p, p}
	}
	return clause, params
}

func filterSearch(s string, o *opts.Opts) (clause string, params []any) {
	if o.TextObj {
		clause = "o.text LIKE ?"
	} else {
		clause = "EXISTS (SELECT 1 FROM textobj WHERE entry = e.id AND text LIKE ?)"
	}
	params = []any{"%" + s + "%"}
	return clause, params
}

// Filters - Filters definitions
var filters = []filter{
	filterVar,
	filterAuthor,
	filterTag,
	filterSearch,
}

// Parse - Build filters from arguments
func Parse(args []string, o *opts.Opts) (clauses []string, params []any) {
	var operators = [2]string{"AND", ""}
	for _, i := range args {
		i = strings.TrimSpace(i)
		// Logical operators
		switch i {
		case "or":
			operators[0] = "OR"
			continue
		case "and":
			operators[0] = "AND"
			continue
		case "not":
			operators[1] = "NOT"
			continue
		}
		// Iterate over filters until one returns a non-empty clause
		for _, fn := range filters {
			fClause, fParams := fn(i, o)
			if fClause != "" {
				clauses = append(clauses, operators[0], operators[1], fClause)
				params = append(params, fParams...)
				break
			}
		}
		// After a filter, reinit logical operators
		operators[0], operators[1] = "AND", ""
	}
	if len(clauses) > 0 {
		clauses[0] = ""
	}
	return clauses, params
}
