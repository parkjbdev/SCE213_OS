echo "STCF Test"
for test in ./testcases/*; do
  ./sched -Sq $test 1> /dev/null 2>/dev/null
  if [ $? -eq 0 ]; then
    echo "✅  passed: $test"
  else
    echo "❌  failed: $test"
  fi
done