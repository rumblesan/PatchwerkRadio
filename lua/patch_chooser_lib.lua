require 'lfs'

math.randomseed(os.time())

function sleep(n)
  os.execute("sleep " .. tonumber(n))
end

function get_sub_directories(directory)
  dirs = {}
  for file in lfs.dir(directory) do
    if file ~= "." and file ~= ".." then
      table.insert(dirs, directory .. "/" .. file)
    end
  end
  return dirs
end

function get_patch_main_files(patch_dir)
  main_patches = {}
  for k,dir in pairs(get_sub_directories(patch_dir)) do
    for k,pd in pairs(get_sub_directories(dir)) do
      main_patch_file = pd .. "/main.pd"
      if lfs.attributes(main_patch_file, "mode") == "file" then
        table.insert(main_patches, main_patch_file)
      end
    end
  end
  return main_patches
end

function select_random_patch_main(patch_dir)
  main_files = get_patch_main_files(patch_dir)
  return main_files[ math.random( #main_files) ]
end

function string:split(sep)
   local sep, fields = sep or ":", {}
   local pattern = string.format("([^%s]+)", sep)
   self:gsub(pattern, function(c) fields[#fields+1] = c end)
   return fields
end


function get_patch_info(patch_path)
  segments = patch_path:split("/")
  return {
    creator = segments[#segments-1],
    title = segments[#segments-2],
    path = patch_path
  }
end
