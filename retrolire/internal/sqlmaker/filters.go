// Package sqlmaker - Build SQL statements
package sqlmaker

import (
	"strings"

	"retrolire/internal/aka"
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

func filterVar(s string, t *aka.Thesaurus) (clause string, params []any, ok bool) {
	idx := strings.Index(s, FieldValueSep)
	if idx > 0 {
		key, value := s[:idx], s[idx+1:]
		if key == "author" {
			return filterAuthor(AuthorPrefix+value, t)
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

func filterAuthor(s string, _ *aka.Thesaurus) (string, []any, bool) {
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

func filterClass(s string, _ *aka.Thesaurus) (string, []any, bool) {
	if strings.HasPrefix(s, ClassPrefix) {
		return "e.class = ?", []any{obj.FromName(s[len(ClassPrefix):])}, true
	}
	return "", nil, false
}

func filterTag(s string, t *aka.Thesaurus) (clause string, params []any, ok bool) {
	const pfx = TagPrefix
	const lp = len(pfx)
	const HasTag = "e.tags ->> ? IS NOT NULL"
	if strings.HasPrefix(s, pfx) {
		tag := strings.TrimSpace(s[lp:])
		var b strings.Builder
		b.WriteRune('(')
		b.WriteString(HasTag)
		params = append(params, tag)
		tagAliases := t.Tags.Desc(tag)
		for _, a := range tagAliases {
			b.WriteString(" OR ")
			b.WriteString(HasTag)
			params = append(params, a)
		}
		b.WriteRune(')')
		return b.String(), params, true
	}
	return clause, params, false
}

func filterSearch(s string, _ *aka.Thesaurus) (clause string, params []any, ok bool) {
	return "e.main LIKE ?", []any{"%" + s + "%"}, true
}
