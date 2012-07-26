function generate(cname, maskname)
   local availableformat = {}
   print(string.format([[
static int sndfile_%s_string2number(const char *str)
{]], cname))

   for line in io.lines(string.format("%s.txt", cname)) do
     local sf, number, comment = line:match('^%s*(%S+)%s+%=%s+(%S+)%,%s+(.*)$')
     print(string.format([[
  if(!strcmp(str, "%s")) %s
    return %s;
]], sf:gsub('SF_FORMAT_', ''):gsub('SF_ENDIAN_', ''):gsub('_', ''), comment, sf))
     table.insert(availableformat, string.format('"%s: %s\\n"',
                                                 sf:gsub('SF_FORMAT_', ''):gsub('SF_ENDIAN_', ''):gsub('_', ''),
                                                 comment:gsub('%/%*', ''):gsub('%*%/', ''):gsub('^%s+', ''):gsub('%s+$', '')))
   end

   print([[
  return -1;
}
]])
            
  print(string.format([[
static const char* sndfile_%s_number2string(int format)
{
  format = format & SF_FORMAT_%s;
]], cname, maskname))

   for line in io.lines(string.format("%s.txt", cname)) do
     local sf, number, comment = line:match('^%s*(%S+)%s+%=%s+(%S+)%,%s+(.*)$')
     print(string.format([[
  if(format == %s) %s
    return "%s";
]], sf, comment, sf:gsub('SF_FORMAT_', ''):gsub('SF_ENDIAN_', ''):gsub('_', '')))
   end
                                               
   print([[
  return NULL;
}
]])

  print(string.format([[
static int sndfile_%slist(lua_State *L)
{
  lua_newtable(L);
]], cname))

   for line in io.lines(string.format("%s.txt", cname)) do
     local sf, number, comment = line:match('^%s*(%S+)%s+%=%s+(%S+)%,%s+(.*)$')
     print(string.format([[
  lua_pushstring(L, "%s");
  lua_setfield(L, -2, "%s");
]], comment:gsub('%/%*', ''):gsub('%*%/', ''):gsub('^%s+', ''):gsub('%s+$', ''),
    sf:gsub('SF_FORMAT_', ''):gsub('SF_ENDIAN_', ''):gsub('_', '')))
   end
                                               
   print([[
  return 1;
}
]])

  print(string.format([[
static const char* sndfile_%s_available = \
%s;
                      ]], cname, table.concat(availableformat, '\n')))
end

generate("format", "TYPEMASK")
generate("subformat", "SUBMASK")
generate("endian", "ENDMASK")
