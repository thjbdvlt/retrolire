--
-- PostgreSQL database dump
--

-- Dumped from database version 16.4 (Debian 16.4-1.pgdg120+2)
-- Dumped by pg_dump version 16.4 (Debian 16.4-1.pgdg120+2)

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
        '"' || name || '[@' || entry || ']"'
    else 
        '"' || name || '[@' || entry || ', p.' || page || ']"'
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
-- Name: get_concepts(text); Type: FUNCTION; Schema: public; Owner: -
--

CREATE FUNCTION public.get_concepts(text) RETURNS TABLE(concept text, definition text, page text)
    LANGUAGE sql
    AS $_$
with matches as (
    select
        regexp_matches($1, '([^\n]+\n\n?):([^\n]*)', 'g') as dl
)
select
    trim(m.dl[1], E'\n\t ') as concept,
    trim(x.quote, E'\n\t ') as definition,
    x.page_number as page
from matches m, split_quote_page_number(trim(m.dl[2], E'\n\t ')) as x;
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
-- Name: get_quotes(text); Type: FUNCTION; Schema: public; Owner: -
--

CREATE FUNCTION public.get_quotes(text) RETURNS TABLE(quote text, page_number text)
    LANGUAGE sql
    AS $_$
with _quotes as (
    select replace(trim(unnest(
        regexp_matches(regexp_replace($1, '^', E'\n'),
            '(\n>.*?(?=\n\n|$))', 'g')
    ), E' \n\t>'), E'\n>', E'\n') as quote
)
select (split_quote_page_number(q.quote)).*
from _quotes q
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
    -- quotes
delete from quote
where entry = new.id;
insert into quote (entry, quote, page)
select new.id, (get_quotes(new.notes)).*
except
select entry, quote, page from quote
where quote.entry = new.id
on conflict do nothing;
    -- concepts
delete from concept where entry = new.id;
insert into concept (entry, name, definition, page)
select new.id, (get_concepts(new.notes)).*;
    -- returns row
return new;
end;
$$;


--
-- Name: quote; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public.quote (
    id integer NOT NULL,
    entry text NOT NULL,
    quote text NOT NULL,
    page text,
    note text,
    context text
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
    || $1.quote || '[@' || $1.entry ||
    case
        when ($1.page is not null) then ', ' || (
            case
                when $1.page like '%-%' then 'pp.'
                else 'p.'
            end
        ) || $1.page
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
select quote_to_string(q) from quote q where q.id = $1;
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

CREATE FUNCTION public.split_quote_page_number(text) RETURNS TABLE(quote text, page_number text)
    LANGUAGE sql
    AS $_$
select s[1], s[2] from regexp_matches(
    -- first: normalize the quote.
    regexp_replace(
        $1,
        '[\[\(](?:pp?\.?|pages?)? *(\d+(?:-\d+)?)[\]\)]\s*$',
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
-- Name: _head; Type: VIEW; Schema: public; Owner: -
--

CREATE VIEW public._head AS
 SELECT id,
    COALESCE(title, url) AS title,
    public.jsonb_concat_values(COALESCE(author, editor, translator), ' '::text) AS someone
   FROM public.entry e;


--
-- Name: _quote; Type: VIEW; Schema: public; Owner: -
--

CREATE VIEW public._quote AS
 SELECT id AS quote_id,
    id AS entry_id,
    quote
   FROM public.quote q;


--
-- Name: concept; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public.concept (
    id integer NOT NULL,
    entry text NOT NULL,
    name text NOT NULL,
    definition text,
    page text
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
-- Name: reading; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public.reading (
    id text NOT NULL,
    notes text,
    abstract text,
    lastedit timestamp without time zone DEFAULT now() NOT NULL
);


--
-- Name: relation; Type: TABLE; Schema: public; Owner: -
--

CREATE TABLE public.relation (
    id integer NOT NULL,
    sujet text NOT NULL,
    objet text NOT NULL,
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
    ADD CONSTRAINT relation_sujet_objet_type_key UNIQUE (sujet, objet, type);


--
-- Name: tag tag_entry_tag_key; Type: CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.tag
    ADD CONSTRAINT tag_entry_tag_key UNIQUE (entry, tag);


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
-- Name: relation_objet_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX relation_objet_idx ON public.relation USING btree (objet);


--
-- Name: relation_sujet_idx; Type: INDEX; Schema: public; Owner: -
--

CREATE INDEX relation_sujet_idx ON public.relation USING btree (sujet);


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
    ADD CONSTRAINT relation_objet_fkey FOREIGN KEY (objet) REFERENCES public.entry(id) ON DELETE CASCADE;


--
-- Name: relation relation_sujet_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.relation
    ADD CONSTRAINT relation_sujet_fkey FOREIGN KEY (sujet) REFERENCES public.entry(id) ON DELETE CASCADE;


--
-- Name: tag tag_entry_fkey; Type: FK CONSTRAINT; Schema: public; Owner: -
--

ALTER TABLE ONLY public.tag
    ADD CONSTRAINT tag_entry_fkey FOREIGN KEY (entry) REFERENCES public.entry(id) ON DELETE CASCADE;


--
-- PostgreSQL database dump complete
--

