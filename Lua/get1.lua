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
-- This program demonstrates the use of TLS in Lua.  TLS is handling
-- the network connection on our behalf.
--
-- *****************************************************************

local tls = require "org.conman.tls"

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
    return read(ctx,len)
  else
    return str
  end
end

-- *****************************************************************

if #arg == 0 then
  io.stderr:write(string.format("usage: %s host resource\n",arg[0]))
  os.exit(1)
end

local config = tls.config()
local ctx    = tls.client()

okay(config,config:protocols "all")
okay(ctx,ctx:configure(config))
okay(ctx,ctx:connect(arg[1],"https"))
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
