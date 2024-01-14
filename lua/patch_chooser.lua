function sleep(n)
  os.execute("sleep " .. tonumber(n))
end

print(chooser_config.count)
for i=0, chooser_config.count, 1
do
  print("looping: " .. i)
  sleep(2)
end
