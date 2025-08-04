// Package sqlmaker - Build SQL statements
package sqlmaker

import (
	"strings"

	"retrolire/internal/config"
	"retrolire/internal/obj"
)

// Filter syntax
const (
	FieldValueSep = ":" // author:antin
	ValuesSep     = "," // author:antin,quintane
	TagPrefix     = "." // .computing .sociology
	AuthorPrefix  = "@" // @wittgenstein
	ClassPrefix   = "=" // =concept =quote =entry
)

func filterVar(s string) (clause string, params []any, ok bool) {
	idx := strings.Index(s, FieldValueSep)
	if idx > 0 {
		key, value := s[:idx], s[idx+1:]
		if key == "author" {
			return filterAuthor(AuthorPrefix + value)
		}
		aliasedKey, ok := config.AliasesCSL[key]
		if ok {
			key = aliasedKey
		}
		values := strings.Split(value, ValuesSep)
		var clauses []string
		for _, v := range values {
			v = strings.TrimSpace(v)
			clauses = append(clauses, "e.csl ->> ? LIKE ?")
			params = append(params, key, "%"+v+"%")
		}
		clause = strings.Join(clauses, " OR ")
		clause = "(" + clause + ")"
		return clause, params, true
	}
	return clause, params, false
}

func filterAuthor(s string) (string, []any, bool) {
	var params []any
	var clause string
	if strings.HasPrefix(s, AuthorPrefix) {
		value := s[len(AuthorPrefix):]
		values := strings.Split(value, ValuesSep)
		var clauses []string
		for _, v := range values {
			v = strings.TrimSpace(v)
			clauses = append(clauses, "e.author LIKE ?")
			params = append(params, "%"+v+"%")
		}
		clause = strings.Join(clauses, " OR ")
		clause = "(" + clause + ")"
		return clause, params, true
	}
	return "", nil, false
}

func filterClass(s string) (string, []any, bool) {
	if strings.HasPrefix(s, ClassPrefix) {
		return "e.class = ?", []any{obj.FromName(s[len(ClassPrefix):])}, true
	}
	return "", nil, false
}

func filterTag(s string) (clause string, params []any, ok bool) {
	const pfx = TagPrefix
	const lp = len(pfx)
	if strings.HasPrefix(s, pfx) {
		clause = "e.tags ->> ? = 1"
		p := strings.TrimSpace(s[lp:])
		return clause, []any{p}, true
	}
	return clause, params, false
}

func filterSearch(s string) (clause string, params []any, ok bool) {
	return "e.main LIKE ?", []any{"%" + s + "%"}, true
}
