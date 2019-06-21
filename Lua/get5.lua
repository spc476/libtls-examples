#!/usr/bin/env lua

local tls = require "org.conman.tls"
local url = require "org.conman.parsers.url"

-- ************************************************************

local function format_request(method,path,query,version)
  query   = query   or ""
  version = version or "1.1"
  
  return string.format("%s /%s HTTP/%s\r\n",method,table.concat(path,"/"),version)
end

-- ************************************************************

local function format_headers(hdrs)
  local res = ""
  for name,value in pairs(hdrs) do
    res = res .. name .. ": " .. value .. "\r\n"
  end
  return res
end

-- ************************************************************

local loc = url:match(arg[1] or "https://www.google.com/")
if not loc then os.exit(1) end

local config = tls.config()
local ctx    = tls.client()

config:protocols "all"
ctx:configure(config)

if not ctx:connect(loc.host,loc.port) then
  print(ctx:error())
  os.exit(1)
end

ctx:handshake()

io.stderr:write(
	format_request("GET",loc.path) ..
	format_headers {
		['Host']       = loc.host,
		['User-Agent'] = "TLSTester/1.0 (TLS Testing Program Lua)",
		['Connection'] = 'close',
		['Accept']     = "*/*",
	} ..
	"\r\n"
)

io.stderr:write("Version:    " , ctx:conn_version() or "","\n")
io.stderr:write("Subject:    " , ctx:peer_cert_subject() or "","\n")
io.stderr:write("Issuer:     " , ctx:peer_cert_issuer() or "","\n")
io.stderr:write("Cipher:     " , ctx:conn_cipher() or "","\n")
io.stderr:write("ALPN:       " , ctx:conn_alpn_selected() or "","\n")
io.stderr:write("Server:     " , ctx:conn_servername() or "","\n")
io.stderr:write("Not-Before: " , os.date("%c",ctx:peer_cert_notbefore()),"\n")
io.stderr:write("Not-After:  " , os.date("%c",ctx:peer_cert_notafter()),"\n")
io.stderr:write("Hash:       " , ctx:peer_cert_hash() or "","\n")
io.stderr:write("\n")

ctx:write(
	format_request("GET",loc.path) ..
	format_headers {
		['Host']       = loc.host,
		['User-Agent'] = "TLSTester/1.0 (TLS Testing Program Lua)",
		['Connection'] = 'close',
		['Accept']     = "*/*",
	} ..
	"\r\n"
)

repeat
  local bytes = ctx:read(tls.BUFFERSIZE)
  io.stdout:write(bytes)
until bytes == ""
