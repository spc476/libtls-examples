-- *****************************************************************
-- Copyright 2018 by Sean Conner.  All Rights Reserved.
--
-- This program is free software; you can redistribute it and/or modify
-- it under the terms of the GNU General Public License as published by
-- the Free Software Foundation; either version 2 of the License, or
-- (at your option) any later version.
--
-- This program is distributed in the hope that it will be useful,
-- but WITHOUT ANY WARRANTY; without even the implied warranty of
-- MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
-- GNU General Public License for more details.
--
-- You should have received a copy of the GNU General Public License
-- along with this program; if not, write to the Free Software
-- Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
--
-- Comments, questions and criticisms can be sent to: sean@conman.org
--
-- This program demonstrates the Lua code managing the socket, leaving
-- TLS to just encryption/decryption.
--
-- *****************************************************************
-- luacheck: ignore 611

local tls = require "org.flummux.tls"
local net = require "org.conman.net"

local g_fpin  = io.open("input.bin","wb")
local g_fpout = io.open("output.bin","wb")

-- *****************************************************************

local function okay(obj,value)
  if not value then
    print(">>>",obj:error())
    os.exit(1)
  end
  return value
end

-- *****************************************************************

local function write(ctx,str)
  local bytes = ctx:write(str)
  if bytes == tls.ERROR then
    return false
  elseif bytes == tls.WANT_INPUT or bytes == tls.WANT_OUTPUT then
    return write(ctx,str)
  else
    return true
  end
end

-- *****************************************************************

local function read(ctx,len)
  local str,strlen = ctx:read(len)
  if strlen == tls.ERROR then
    return nil
  elseif strlen == tls.WANT_INPUT or strlen == tls.WANT_OUTPUT then
    return read(ctx,str)
  else
    return str
  end
end

-- *****************************************************************

local function connect_to_host(host,port)
  local addrlist = net.address2(host,'ip','tcp',port)
  if addrlist then
    local sock = net.socket('ip','tcp')
    if sock then
      local conn = { sock = sock , inbuf = "" }
      if sock:connect(addrlist[1]) == 0 then
        return conn
      end
    end
  end
  return false
end

-- *****************************************************************

local function cb_read(_,len,sock)
  if #sock.inbuf == 0 then
    local _,data = sock.sock:recv()
    if not data then return nil,tls.ERROR end
    g_fpin:write(data)
    sock.inbuf = data
  end
  
  local ret = sock.inbuf:sub(1,len)
  sock.inbuf = sock.inbuf:sub(len+1,-1)
  return ret,0
end

-- *****************************************************************

local function cb_write(_,str,sock)
  g_fpout:write(str)
  local bytes = sock.sock:send(nil,str)
  if not bytes then bytes = tls.ERROR end
  return bytes
end

-- *****************************************************************

if #arg == 0 then
  io.stderr:write(string.format("usage: %s host resource\n",arg[0]))
  os.exit(1)
end

local config = tls.config()
local ctx    = tls.client()

okay(config,config:set_protocols "all")
okay(ctx,ctx:configure(config))

local sock = okay(ctx,connect_to_host(arg[1],"https"))

okay(ctx,ctx:connect_cbs(arg[1],sock,cb_read,cb_write))
okay(ctx,write(ctx,string.format(
     "GET %s HTTP/1.1\r\n"
  .. "Host: %s\r\n"
  .. "User-Agent: TLSTester/1.0 (TLS Testing Program Lua)\r\n"
  .. "Connection: close\r\n"
  .. "Accept: */*\r\n"
  .. "\r\n",
     arg[2],
     arg[1]
)))

while true do
  local bytes = okay(ctx,read(ctx,1024))
  if bytes == "" then break end
  io.stdout:write(bytes)
end
