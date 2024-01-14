require 'lua.patch_chooser_lib'

print("Patch Chooser Loaded")
print(" patch dir is " .. AppConfig.patch_folder)
print(" looping " .. AppConfig.load_count .. " times")
for i=0, AppConfig.load_count, 1
do
  random_patch = select_random_patch_main(AppConfig.patch_folder)
  patch_info = get_patch_info(random_patch)
  send_load_patch(AppConfig.state, patch_info)
  sleep(30)
end
