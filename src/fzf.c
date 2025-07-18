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
#include "fzf.h"

const char* fzf_base[] = {
  "fzf",
  "--read0", // record separator '\0'
  "-0",      // cancel if no result
  "--no-multi", // select only one entry
  "--tiebreak", // algorithm
  "begin",
  "--wrap", // wrap lines
  // "--reverse", // top-down
  "--bind",
  "?:toggle-preview", // toggle the preview (informations)
  // visual options
#ifdef FZF_MARGIN
  "--margin",
  FZF_MARGIN,
#endif
#ifdef FZF_PADDING
  "--padding",
  FZF_PADDING,
#endif
#ifdef FZF_TAB_STOP
  "--tabstop",
  FZF_TAB_STOP,
#endif
#ifdef FZF_SET_CYCLE
  "--cycle",
#endif
#ifdef FZF_SET_CYCLE
  "--reverse",
#endif
#ifdef FZF_COLORS
  "--color",
  FZF_COLORS,
#endif
#ifndef FZF_SET_MOUSE
  "--no-mouse",
#endif
#ifndef FZF_SET_BOLD
  "--no-bold",
#endif
#ifndef FZF_SET_SEPARATOR
  "--no-separator",
#endif
  NULL,
};

// TODO: this could be less repeated...
const char* fzf_idea[] = {
  "--gap",
  "1",
  "--preview-window",
  "bottom,4,nohidden",
  "--preview",
  "retrolire _head {-1}",
  "--bind",
  "enter:become(echo -n {-1} {-3})",
  "--bind",
  "::execute(retrolire _input {-1} {-2})",
  NULL,
};

const char* fzf_quote[] = {
  "--gap",
  "1",
  "--preview-window",
  "bottom,4,nohidden",
  "--preview",
  "retrolire _head {-1}",
  "--bind",
  "enter:become(echo -n {-3})",
  "--bind",
  "::execute(retrolire _input {-1} {-2})",
  NULL,
};

const char* fzf_concept[] = {
  "--preview-window",
  "bottom,6,nohidden",
  "--bind",
  "enter:become(echo -n {-3})",
  "--with-nth",
  "1",
  "--info-command",
  "echo {-1}",
  "--preview",
  "echo {2} && echo; retrolire _head {-1}",
  "--bind", // command call using `:`, e.g. `:open`.
  "::execute(retrolire _input {-1} {-2})",
  NULL,
};

const char* fzf_entry[] = {
#ifdef FZF_PREVIEW_POS
  "--preview-window",
  FZF_PREVIEW_POS, // from config.h
#endif
  "--preview",
  "retrolire _preview {-1} | bat -l md -p --color=always",
  "--bind", // keybinding to select an entry
  "enter:become(echo -n {-1})",
  "--bind",
  "::execute(retrolire _input {-1})",
  NULL,
};

// wrap-sign and delimiter (command-specific)
const char* fzf_multiline[] = {
#ifdef FZF_WRAP_SIGN
  "--wrap-sign",
  // LSP may underline this, but there is NO missing comma.
  "\t" FZF_WRAP_SIGN,
#endif
  "-d",
  "\n\t",
  NULL,
};

const char* fzf_oneline[] = {
#ifdef FZF_WRAP_SIGN
  "--wrap-sign",
  FZF_WRAP_SIGN,
#endif
  "-d",
  "\t",
  NULL,
};
