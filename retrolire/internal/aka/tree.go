// Package aka - Defines Also Known AS (i.e. alias) type
package aka

import (
	"bufio"
	"os"
	"strings"
)

// AKA - A tree representing aliases and subaliases
type AKA map[string]*node

type node struct {
	indent   int
	name     string
	aliases  []string
	parent   *node
	children []*node
}

func parseLine(line string) *node {
	var name string
	var aliases []string
	indent := getIndent(line)
	const AliasDelimiter = " = " // Space around equal sign is required
	if strings.Contains(line, AliasDelimiter) {
		parts := strings.Split(line, AliasDelimiter)
		name = parts[0]
		aliases = make([]string, len(parts)-1)
		for i, a := range parts[1:] {
			aliases[i] = strings.TrimSpace(a)
		}
	} else {
		name = line
	}
	name = strings.TrimSpace(name)
	return &node{name: name, aliases: aliases, indent: indent}
}

func parseTree(file *os.File) *AKA {
	scanner := bufio.NewScanner(file)
	var maxIndent int
	var nodes []*node
	for scanner.Scan() {
		line := scanner.Text()
		t := parseLine(line)
		t.indent = getIndent(line)
		if t.indent > maxIndent {
			maxIndent = t.indent
		}
		nodes = append(nodes, t)
	}
	tr := AKA{}
	if len(nodes) == 0 {
		return &tr
	}
	prevs := make([]*node, maxIndent+1)
	prevs[0] = nodes[0]
	for i := 1; i < len(nodes)-1; i++ {
		t := nodes[i]
		prevTag := nodes[i-1]
		switch {
		case t.indent == prevTag.indent:
			t.parent = prevTag.parent
		case t.indent > prevTag.indent:
			t.parent = prevTag
		case t.indent < prevTag.indent:
			t.parent = prevs[t.indent]
		}
		t.parent.children = append(t.parent.children, t)
		prevs[t.indent] = t
		tr[t.name] = t
	}
	return &tr
}

func akaDesc(n *node) []string {
	aliases := []string{n.name}
	for _, c := range n.children {
		aliases = append(aliases, c.aliases...)
		aliases = append(aliases, akaDesc(c)...)
	}
	return aliases
}

func akaAsc(n *node) []string {
	aliases := []string{n.name}
	aliases = append(aliases, n.aliases...)
	if n.parent != nil {
		aliases = append(aliases, akaAsc(n.parent)...)
	}
	return aliases
}

// Desc - Get subclasses and aliases
func (t *AKA) Desc(name string) []string {
	n, ok := (*t)[name]
	if !ok {
		return []string{}
	}
	return akaDesc(n)[1:] // Remove the name itself
}

// Asc - Get superclasses and aliases
func (t *AKA) Asc(name string) []string {
	n, ok := (*t)[name]
	if !ok {
		return []string{}
	}
	return akaAsc(n)[1:] // Remove name itself
}
