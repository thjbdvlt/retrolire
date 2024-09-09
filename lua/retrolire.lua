local fzf_base_opts = [[ --read0 -d '\n\t' --wrap ]]

local function put_at_cursor(s)
    -- insert text at the cursor
    --
    local pos = vim.api.nvim_win_get_cursor(0)
    local pos_col = pos[2]
    local row, col = pos[1] - 1, pos_col
    vim.api.nvim_buf_set_text(0, row, col, row, col, { s })
    local len = vim.fn.strchars(s)
    vim.api.nvim_win_set_cursor(0, { row + 1, (col + len) - 1 })
end


function RetroCite(args)
    -- edit a file selected with fzf.
    --
    -- if args == nil then args = '' end
    if args == nil then args = {args = ''} end
    local cmd = "retrolire cite -o " .. args.args
    coroutine.wrap(
        function()
            local fzf = require('fzf')
            local fzf_opts = fzf_base_opts ..
                [[ --bind='enter:become(echo {1})' --tiebreak=begin ]]
            local result = fzf.fzf(cmd, fzf_opts)
            if result then
                local entry_id = result[1]
                entry_id = '[@' .. entry_id .. ']'
                put_at_cursor(entry_id)
            end
        end
    )()
end

function RetroQuote(args)
    -- edit a file selected with fzf.
    --
    if args == nil then args = {args = ''} end
    local cmd = "retrolire quote -o " .. args.args
    local fzf_opts = fzf_base_opts ..
        [[ --preview='retrolire _head {2}']] ..
        [[ --preview-window='bottom,5' ]] ..
        [[ --bind='enter:become(retrolire _quote {1})' ]]
    coroutine.wrap(
        function()
            local fzf = require('fzf')
            local result = fzf.fzf(cmd, fzf_opts)
            if result then
                local quote = result[1]
                put_at_cursor(quote)
            end
        end
    )()
end

-- Commands
-- all arguments will be passed to retrolire, so it's possible
-- to do something like:
-- :Cite -t ordinaire -v author=antin
vim.api.nvim_create_user_command(
    'Cite', RetroCite, {
        desc = 'insert an entry id using retrolire',
        nargs = '*'
    }
)
vim.api.nvim_create_user_command(
    'Quote', RetroQuote, {
        desc = 'insert a quote using retrolire.',
        nargs = '*'
    }
)
