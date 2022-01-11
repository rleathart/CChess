local win64 = jit.os == "Windows"
local map = vim.api.nvim_set_keymap

local map_opts = {
  silent = true,
  noremap = true,
}

if win64 then
  map('n', "<A-r>", "<Cmd>call jobstart('start build\\win64_main.exe')<CR>", map_opts)
end

vim.opt.foldmethod = 'marker'

local type_strings = {
  'byte', 'b32', 'f32', 'f64', 'u8', 's8', 'u16', 's16', 'u32', 's32', 'u64', 's64', 'umm',
  'recti', 'v2i', 'program_input', 'loaded_bitmap', 'move', 'piece', 'move_array',
}

vim.cmd(string.format([[
augroup LOCAL_RC
  au!
  au Syntax c,cpp syn keyword cType %s
  au Syntax c,cpp syn keyword cStorageClass global local internal
  au Syntax c,cpp syn match cType display /\<platform_[a-z0-9_]*\>/
augroup END
]], table.concat(type_strings, ' '))
)
