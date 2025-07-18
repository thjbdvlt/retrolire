/* retrolire -- commande line bibliography manager.
 * Copyright (C) 2024,2025  thjbdvlt
 *
 * retrolire is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * retrolire is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with retrolire.  If not, see <https://www.gnu.org/licenses/>.
 */

--
-- PostgreSQL database dump
--

-- Dumped from database version 16.8 (Debian 16.8-1.pgdg120+1)
-- Dumped by pg_dump version 16.8 (Debian 16.8-1.pgdg120+1)

SET statement_timeout = 0;
SET lock_timeout = 0;
SET idle_in_transaction_session_timeout = 0;
SET client_encoding = 'UTF8';
SET standard_conforming_strings = on;
SELECT pg_catalog.set_config('search_path', '', false);
SET check_function_bodies = false;
SET xmloption = content;
SET client_min_messages = warning;
SET row_security = off;

--
-- Name: cite_concept(integer); Type: FUNCTION; Schema: public; Owner: -
--

CREATE FUNCTION public.cite_concept(concept_id integer) RETURNS text
    LANGUAGE sql
    AS $_$
select
    case when page is null then
        '"' || s || '[@' || entry || ']"'
    else 
        '"' || s || '[@' || entry || ' ' || page || ']"'
    end
from concept where id = $1
$_$;


--
-- Name: field_exists(text); Type: FUNCTION; Schema: public; Owner: -
--

CREATE FUNCTION public.field_exists(field text) RETURNS boolean
    LANGUAGE sql
    AS $_$
select exists (
    select 1
    from information_schema.columns
    where table_schema = 'public'
    and table_name = 'entry'
    and column_name = $1
);
$_$;


--
-- Name: get_block_linenr(text); Type: FUNCTION; Schema: public; Owner: -
--

CREATE FUNCTION public.get_block_linenr(text) RETURNS TABLE(s text, ln integer)
    LANGUAGE sql
    AS $_$
    WITH paragraphs AS (
        SELECT
            regexp_split_to_table($1, E'\n\n') AS par
),
paragraphs_numbered AS (
    SELECT
        row_number() OVER (),
        x.par,
        regexp_count (x.par, E'\n') AS count_nl
    FROM
        paragraphs x
),
paragraphs_lines_count AS (
    SELECT
        y.par,
        y.row_number,
        y.count_nl AS _count_nl,
        CASE WHEN row_number > 1 THEN
            y.count_nl + 2
        ELSE
            y.count_nl + 1
        END AS count_nl
    FROM
        paragraphs_numbered y
)
SELECT
    y.par,
    sum(count_nl) OVER (ORDER BY row_number) - _count_nl
    FROM
        paragraphs_lines_count y;
$_$;


--
-- Name: get_block_typed(text); Type: FUNCTION; Schema: public; Owner: -
--

CREATE FUNCTION public.get_block_typed(text) RETURNS TABLE(s text, linenr integer, page text, type text)
    LANGUAGE sql
    AS $_$

-- first, get all blocks. because all objects are blocks
WITH x AS (
    SELECT
        (get_block_linenr ($1)).*
),

-- then, get the page number at the end of each blocks
paged AS (
SELECT
    -- trim the string
    trim(n.s, E'\t\n\r ') AS s,
    x.ln AS linenr,
    n.page_number AS page
FROM
    x,
    split_quote_page_number (x.s) n
)

-- then, put an attribute to each block (object)
SELECT
    x.s,
    x.linenr,
    x.page,
    case when regexp_like(x.s, '^>') then 'quote'
    when regexp_like(x.s, '\n\s*:') then 'concept'
    when regexp_like(x.s, '^#+ |\n[=-]{2,}$') then 'heading'
    when regexp_like(x.s, '^\(') then 'parenthese'
    when regexp_like(x.s, '^[-=]+$') then 'rule'
    when regexp_like(x.s, '^\s*$') then 'empty'
    else 'idea'
    end as type
from paged x;
$_$;


--
-- Name: get_concepts(text); Type: FUNCTION; Schema: public; Owner: -
--

CREATE FUNCTION public.get_concepts(text) RETURNS TABLE(concept text, definition text, page text)
    LANGUAGE sql
    AS $_$
    WITH matches AS (
        SELECT
            regexp_matches($1, '([^\n]+\n\n?):([^\n]*)', 'g') AS dl
)
    SELECT
        trim(m.dl[1], E'\n\t ') AS concept,
        trim(x.quote, E'\n\t ') AS definition,
        x.page_number AS page
    FROM
        matches m,
        split_quote_page_number (trim(m.dl[2], E'\n\t ')) AS x;
$_$;


--
-- Name: get_notes_with_yaml(text); Type: FUNCTION; Schema: public; Owner: -
--

CREATE FUNCTION public.get_notes_with_yaml(entry_id text) RETURNS text
    LANGUAGE sql
    AS $_$
select
case when
    not regexp_like(trim(r.notes), '^---\nid:') then
E'---\nid: ' || h.id
|| E'\ntitle: ' || h.title
|| E'\nsomeone: ' || h.someone
|| e'\n---\n\n' || r.notes
else r.notes
end
from _head h join reading r on r.id = h.id where h.id = $1;
$_$;


--
-- Name: get_paragraphs(text); Type: FUNCTION; Schema: public; Owner: -
--

CREATE FUNCTION public.get_paragraphs(text) RETURNS TABLE(paragraph text)
    LANGUAGE sql
    AS $_$
    WITH split AS (
        SELECT
            regexp_split_to_table($1, E'\n\n+') AS s
)
    SELECT
        s
    FROM
        split
    WHERE
        NOT regexp_like (s, E'(^|\n)[>#:\[]')
        AND NOT regexp_like (s, '^\s*\(')
        AND NOT regexp_like (s, '(^|\n)\s*[-=]+\s*(^|$)')
$_$;


--
-- Name: get_quotes(text); Type: FUNCTION; Schema: public; Owner: -
--

CREATE FUNCTION public.get_quotes(text) RETURNS TABLE(idea text, linenr integer, page text)
    LANGUAGE sql
    AS $_$
    WITH x AS (
        SELECT
            (get_block_linenr ($1)).*
)
    SELECT
        regexp_replace(trim(x.s, E'\t\n\r '), E'\n', E'\n\t', 'g') AS idea,
        x.ln AS linenr,
        n.page_number AS page
    FROM
        x,
        split_quote_page_number (x.s) n
WHERE
    x.s IS NOT NULL
    AND regexp_like (x.s, '^\s*>')
$_$;


SET default_tablespace = '';

SET default_table_access_method = heap;

--
-- Name: entry; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public.entry (
    id text NOT NULL,
    title text,
    "container-title" text,
    type text,
    url text,
    isbn text,
    doi text,
    publisher text,
    "publisher-place" text,
    keyword text,
    abstract text,
    annote text,
    file text,
    editor jsonb,
    author jsonb,
    "container-author" jsonb,
    translator jsonb,
    "DOI" text,
    "URL" text,
    "chapter-number" text,
    issued jsonb,
    accessed jsonb,
    "collection-title" text,
    "title-short" text,
    page text,
    volume text,
    "ISBN" text,
    edition text,
    "ISSN" text,
    issue text,
    "collection-number" text,
    genre text,
    number text,
    serie text,
    "container-title-short" text,
    version text,
    "container-title-shortw" text,
    "original-date" jsonb,
    CONSTRAINT valid_author CHECK ((((jsonb_typeof(author) = 'array'::text) AND (jsonb_typeof(author[0]) = 'object'::text)) OR (author IS NULL))),
    CONSTRAINT valid_container_author CHECK ((((jsonb_typeof("container-author") = 'array'::text) AND (jsonb_typeof("container-author"[0]) = 'object'::text)) OR ("container-author" IS NULL))),
    CONSTRAINT valid_editor CHECK ((((jsonb_typeof(editor) = 'array'::text) AND (jsonb_typeof(editor[0]) = 'object'::text)) OR (editor IS NULL))),
    CONSTRAINT valid_translator CHECK ((((jsonb_typeof(translator) = 'array'::text) AND (jsonb_typeof(translator[0]) = 'object'::text)) OR (translator IS NULL)))
);


--
-- Name: get_tags(public.entry, text); Type: FUNCTION; Schema: public; Owner: -
--

CREATE FUNCTION public.get_tags(public.entry, delimiter text DEFAULT '+'::text) RETURNS text
    LANGUAGE sql
    AS $_$
select $2 || string_agg(t.tag, ' ' || $2) as tag
from tag t where t.entry = $1.id;
$_$;


--
-- Name: jsonb_array_concat(jsonb, text); Type: FUNCTION; Schema: public; Owner: -
--

CREATE FUNCTION public.jsonb_array_concat(jsonb, text) RETURNS text
    LANGUAGE sql
    AS $_$
select string_agg(x, $2)
from jsonb_array_elements_text($1) as t(x);
$_$;


--
-- Name: jsonb_concat_values(jsonb, text); Type: FUNCTION; Schema: public; Owner: -
--

CREATE FUNCTION public.jsonb_concat_values(jsonb, text) RETURNS text
    LANGUAGE sql
    AS $_$
select string_agg(value, $2)
from (
    select (
        jsonb_each_text(jsonb_array_elements($1))
    ).value
) as t(value)
$_$;


--
-- Name: list_fields(); Type: FUNCTION; Schema: public; Owner: -
--

CREATE FUNCTION public.list_fields() RETURNS text
    LANGUAGE sql
    AS $$
select string_agg(x.column_name, ' ')
from information_schema.columns x
where table_schema = 'public'
and table_name = 'entry';
$$;


--
-- Name: move_reading_fields(); Type: FUNCTION; Schema: public; Owner: -
--

CREATE FUNCTION public.move_reading_fields() RETURNS trigger
    LANGUAGE plpgsql
    AS $$
begin
perform move_things();
return new;
end;
$$;


--
-- Name: move_things(); Type: FUNCTION; Schema: public; Owner: -
--

CREATE FUNCTION public.move_things() RETURNS void
    LANGUAGE sql
    AS $$;
-- insert a reading for every entry.
insert into reading (id) select id from entry on conflict do nothing;;
-- move keyword to table tag
insert into tag (entry, tag)
    select e.id,
    unnest(string_to_array(
            regexp_replace(e.keyword, '\s+', '', 'g'), ','
    ))
from entry e
where e.keyword is not null
on conflict do nothing;;
-- move reading notes (annote)
update reading r set notes = e.annote
from entry e where e.id = r.id
and e.annote is not null;;
-- mote abstract
update reading r set abstract = e.abstract
from entry e where e.id = r.id
and e.abstract is not null;;
-- move filepath
insert into file (entry, filepath)
select id, file from entry
where file is not null
on conflict do nothing;
-- delete values from entry:
update entry set keyword = null;;
update entry set abstract = null;;
update entry set annote = null;;
update entry set file = null;;
$$;


--
-- Name: parse_note(); Type: FUNCTION; Schema: public; Owner: -
--

CREATE FUNCTION public.parse_note() RETURNS trigger
    LANGUAGE plpgsql
    AS $$
begin

delete from quote where entry = new.id;
delete from idea where entry = new.id;
delete from concept where entry = new.id;
delete from relation where subject = new.id;

with typed as (
    select (get_block_typed(new.notes)).*
),
_quote as (
    insert into quote (entry, s, page, linenr)
    select new.id, regexp_replace(typed.s, '(^|\n)>\s*', '', 'g'), typed.page, typed.linenr
    from typed
    where typed.type = 'quote'
),
_idea as (
    insert into idea (entry, s, page, linenr)
    select new.id, typed.s, typed.page, typed.linenr
    from typed
    where typed.type = 'idea'
)
, _splitted_concept as (
    select regexp_matches(typed.s, '([^\n]+\n\n?):([^\n]*)', 'g') as rg, typed.*
    from typed
)
insert into concept (entry, s, definition, page, linenr)
select new.id, s.rg[1], s.rg[2], s.page, s.linenr
from _splitted_concept s
where s.type = 'concept';

-- Test
with _relation as (
    select regexp_matches(new.notes, '\[([^\[\]]*)@(\w+)([^\[\]]*)\]') as rg
)
insert into relation (subject, type, object, comment)
select new.id, x.rg[1], x.rg[2], x.rg[3] from _relation as x;

return new;

end;
$$;


--
-- Name: quote; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public.quote (
    id integer NOT NULL,
    entry text NOT NULL,
    s text NOT NULL,
    page text,
    note text,
    context text,
    linenr integer
);


--
-- Name: quote_to_string(public.quote, boolean, boolean); Type: FUNCTION; Schema: public; Owner: -
--

CREATE FUNCTION public.quote_to_string(public.quote, block boolean DEFAULT false, newlines boolean DEFAULT false) RETURNS text
    LANGUAGE sql
    AS $_$
select
    case
        when block then (
            case
                when newlines then E'\n\n'
                else ''
            end || '> '
        )
        else '"'
    end
    || $1.s || '[@' || $1.entry ||
    case
        when ($1.page is not null) then ' ' || $1.page
        else ''
    end
    || ']' ||
    case
        when block then (
            case
                when newlines then E'\n\n'
                else ''
            end
        )
        else '"'
    end;
$_$;


--
-- Name: quote_to_string_from_id(integer, boolean, boolean); Type: FUNCTION; Schema: public; Owner: -
--

CREATE FUNCTION public.quote_to_string_from_id(integer, block boolean DEFAULT false, newlines boolean DEFAULT false) RETURNS text
    LANGUAGE sql
    AS $_$
    SELECT
        quote_to_string (q, $2)
    FROM
        quote q
    WHERE
        q.id = $1;
$_$;


--
-- Name: short_entry_from_id(text); Type: FUNCTION; Schema: public; Owner: -
--

CREATE FUNCTION public.short_entry_from_id(text) RETURNS text
    LANGUAGE sql
    AS $_$
select
    e.id || E'\n\n' || e.title || E'\n\n' ||
    jsonb_concat_values(coalesce(e.author, e.editor, e.translator), ' ')
from entry e where e.id = $1
$_$;


--
-- Name: split_quote_page_number(text); Type: FUNCTION; Schema: public; Owner: -
--

CREATE FUNCTION public.split_quote_page_number(text) RETURNS TABLE(s text, page_number text)
    LANGUAGE sql
    AS $_$
select s[1], s[2] from regexp_matches(
    -- first: normalize the quote.
    regexp_replace(
        $1,
        '[\[\(](?:pp?\.?|pages?)? *(\d+(?:-\d+)?)[\]\)][[:punct:]]?\s*$',
        '[\1]'
    ),
    -- then, split
    '(.*?)(?:\[(\d+(?:-\d+)?)\])?$'
    ) as x(s)
$_$;


--
-- Name: string_to_tags(text, text); Type: FUNCTION; Schema: public; Owner: -
--

CREATE FUNCTION public.string_to_tags(entry text, tags text) RETURNS void
    LANGUAGE sql
    AS $_$
delete from tag where entry = $1;
insert into tag (entry, tag)
select $1::text, regexp_split_to_table($2::text, '\s*\n\s*')
except
select $1::text, ''::text;
$_$;


--
-- Name: to_csl(public.entry); Type: FUNCTION; Schema: public; Owner: -
--

CREATE FUNCTION public.to_csl(public.entry) RETURNS jsonb
    LANGUAGE sql
    AS $_$
select jsonb_strip_nulls(to_jsonb($1) - 'keyword' - 'abstract' - 'annote');
$_$;


--
-- Name: _cache; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public._cache (
    id integer GENERATED ALWAYS AS (1) STORED,
    file text,
    entry text
);


--
-- Name: file; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public.file (
    id integer NOT NULL,
    entry text,
    filepath text NOT NULL,
    type text,
    note text
);


--
-- Name: reading; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public.reading (
    id text NOT NULL,
    notes text,
    abstract text,
    lastedit timestamp without time zone DEFAULT now() NOT NULL,
    tsvec tsvector
);


--
-- Name: _head; Type: VIEW; Schema: public; Owner: -
--

CREATE VIEW public._head AS
 SELECT e.id,
    COALESCE(e.title, e.url) AS title,
    public.jsonb_concat_values(COALESCE(e.author, e.editor, e.translator), ' '::text) AS someone,
    COALESCE(e."container-title") AS container,
    e."URL" AS url,
    ( SELECT jsonb_agg(f.filepath) AS jsonb_agg
           FROM public.file f
          WHERE (f.entry = e.id)
          GROUP BY e.id) AS file,
    ((NOT ((r.notes IS NULL) OR (TRIM(BOTH ' 
	'::text FROM r.notes) = ''::text))))::integer AS has_notes
   FROM (public.entry e
     JOIN public.reading r ON ((r.id = e.id)));


--
-- Name: _quote; Type: VIEW; Schema: public; Owner: -
--

CREATE VIEW public._quote AS
 SELECT id AS quote_id,
    id AS entry_id,
    s AS quote
   FROM public.quote q;


--
-- Name: authors; Type: VIEW; Schema: public; Owner: -
--

CREATE VIEW public.authors AS
 WITH x AS (
         SELECT DISTINCT jsonb_array_elements(entry.author) AS y
           FROM public.entry
        ), x2 AS (
         SELECT (x.y ->> 'family'::text) AS y
           FROM x
        UNION
         SELECT (x.y ->> 'literal'::text) AS y
           FROM x
        )
 SELECT y AS name
   FROM x2
  WHERE ((y IS NOT NULL) AND (y <> ''::text));


--
-- Name: concept; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public.concept (
    id integer NOT NULL,
    entry text NOT NULL,
    s text NOT NULL,
    definition text,
    page text,
    linenr integer
);


--
-- Name: concept_id_seq; Type: SEQUENCE; Schema: public; Owner: -
--

ALTER TABLE public.concept ALTER COLUMN id ADD GENERATED ALWAYS AS IDENTITY (
    SEQUENCE NAME public.concept_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1
);


--
-- Name: file_id_seq; Type: SEQUENCE; Schema: public; Owner: -
--

ALTER TABLE public.file ALTER COLUMN id ADD GENERATED ALWAYS AS IDENTITY (
    SEQUENCE NAME public.file_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1
);


--
-- Name: idea; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public.idea (
    entry text,
    s text NOT NULL,
    page text,
    linenr integer
);


--
-- Name: quote_id_seq; Type: SEQUENCE; Schema: public; Owner: -
--

ALTER TABLE public.quote ALTER COLUMN id ADD GENERATED ALWAYS AS IDENTITY (
    SEQUENCE NAME public.quote_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1
);


--
-- Name: relation; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public.relation (
    id integer NOT NULL,
    subject text NOT NULL,
    object text NOT NULL,
    type text NOT NULL,
    comment text
);


--
-- Name: relation_id_seq; Type: SEQUENCE; Schema: public; Owner: -
--

ALTER TABLE public.relation ALTER COLUMN id ADD GENERATED ALWAYS AS IDENTITY (
    SEQUENCE NAME public.relation_id_seq
    START WITH 1
    INCREMENT BY 1
    NO MINVALUE
    NO MAXVALUE
    CACHE 1
);


--
-- Name: tag; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public.tag (
    entry text NOT NULL,
    tag text NOT NULL,
    CONSTRAINT no_empty_tag CHECK ((tag <> ''::text))
);


--
-- Name: _cache _cache_id_key; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public._cache
    ADD CONSTRAINT _cache_id_key UNIQUE (id);


--
-- Name: concept concept_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.concept
    ADD CONSTRAINT concept_pkey PRIMARY KEY (id);


--
-- Name: entry entry_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.entry
    ADD CONSTRAINT entry_pkey PRIMARY KEY (id);


--
-- Name: file file_entry_filepath_key; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.file
    ADD CONSTRAINT file_entry_filepath_key UNIQUE (entry, filepath);


--
-- Name: file file_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.file
    ADD CONSTRAINT file_pkey PRIMARY KEY (id);


--
-- Name: quote quote_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.quote
    ADD CONSTRAINT quote_pkey PRIMARY KEY (id);


--
-- Name: reading reading_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.reading
    ADD CONSTRAINT reading_pkey PRIMARY KEY (id);


--
-- Name: relation relation_pkey; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.relation
    ADD CONSTRAINT relation_pkey PRIMARY KEY (id);


--
-- Name: relation relation_sujet_objet_type_key; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.relation
    ADD CONSTRAINT relation_sujet_objet_type_key UNIQUE (subject, object, type);


--
-- Name: tag tag_entry_tag_key; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.tag
    ADD CONSTRAINT tag_entry_tag_key UNIQUE (entry, tag);


--
-- Name: concept_entry_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX concept_entry_idx ON public.concept USING btree (entry);


--
-- Name: entry_publisher_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX entry_publisher_idx ON public.entry USING btree (publisher);


--
-- Name: entry_title_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX entry_title_idx ON public.entry USING btree (title);


--
-- Name: file_entry_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX file_entry_idx ON public.file USING btree (entry);


--
-- Name: file_filepath_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX file_filepath_idx ON public.file USING btree (filepath);


--
-- Name: idea_entry_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX idea_entry_idx ON public.idea USING btree (entry);


--
-- Name: quote_entry_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX quote_entry_idx ON public.quote USING btree (entry);


--
-- Name: reading_id_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX reading_id_idx ON public.reading USING btree (id);


--
-- Name: reading_lastedit_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX reading_lastedit_idx ON public.reading USING btree (lastedit);


--
-- Name: reading_tsvec_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX reading_tsvec_idx ON public.reading USING gin (tsvec);


--
-- Name: relation_objet_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX relation_objet_idx ON public.relation USING btree (object);


--
-- Name: relation_sujet_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX relation_sujet_idx ON public.relation USING btree (subject);


--
-- Name: relation_type_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX relation_type_idx ON public.relation USING btree (type);


--
-- Name: tag_entry_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX tag_entry_idx ON public.tag USING btree (entry);


--
-- Name: tag_tag_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX tag_tag_idx ON public.tag USING btree (tag);


--
-- Name: entry move_fields; Type: TRIGGER; Schema: public; Owner: -
--

CREATE TRIGGER move_fields AFTER INSERT ON public.entry FOR EACH STATEMENT EXECUTE FUNCTION public.move_reading_fields();


--
-- Name: reading parse_note; Type: TRIGGER; Schema: public; Owner: -
--

CREATE TRIGGER parse_note AFTER INSERT OR UPDATE OF notes ON public.reading FOR EACH ROW EXECUTE FUNCTION public.parse_note();


--
-- Name: concept concept_entry_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.concept
    ADD CONSTRAINT concept_entry_fkey FOREIGN KEY (entry) REFERENCES public.entry(id) ON DELETE CASCADE;


--
-- Name: file file_entry_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.file
    ADD CONSTRAINT file_entry_fkey FOREIGN KEY (entry) REFERENCES public.entry(id) ON DELETE CASCADE;


--
-- Name: idea idea_entry_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.idea
    ADD CONSTRAINT idea_entry_fkey FOREIGN KEY (entry) REFERENCES public.entry(id) ON UPDATE CASCADE ON DELETE CASCADE;


--
-- Name: quote quote_entry_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.quote
    ADD CONSTRAINT quote_entry_fkey FOREIGN KEY (entry) REFERENCES public.entry(id) ON DELETE CASCADE;


--
-- Name: reading reading_id_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.reading
    ADD CONSTRAINT reading_id_fkey FOREIGN KEY (id) REFERENCES public.entry(id) ON DELETE CASCADE;


--
-- Name: relation relation_objet_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.relation
    ADD CONSTRAINT relation_objet_fkey FOREIGN KEY (object) REFERENCES public.entry(id) ON DELETE CASCADE;


--
-- Name: relation relation_sujet_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.relation
    ADD CONSTRAINT relation_sujet_fkey FOREIGN KEY (subject) REFERENCES public.entry(id) ON DELETE CASCADE;


--
-- Name: tag tag_entry_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.tag
    ADD CONSTRAINT tag_entry_fkey FOREIGN KEY (entry) REFERENCES public.entry(id) ON DELETE CASCADE;


--
-- PostgreSQL database dump complete
--

