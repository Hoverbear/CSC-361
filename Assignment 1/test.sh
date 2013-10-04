#! /bin/bash

echo "Testing: GET / HTTP/1.0"
echo -e -n "GET / HTTP/1.0\r\n\r\n" | nc -u -w1 127.0.0.1 8080 > tmp
if diff tmp ./test-out/index.html > /dev/null; then
  printf "  \e[32m Pass! \e[0m \n"
else
  printf "  \e[31m Fail! \e[0m \n"
fi

echo "Testing: GET /http.request HTTP/1.0"
echo -e -n "GET /http.request HTTP/1.0\r\n\r\n" | nc -u -w1 127.0.0.1 8080 > tmp
if diff tmp ./test-out/http.request > /dev/null; then
  printf "  \e[32m Pass! \e[0m \n"
else
  printf "  \e[31m Fail! \e[0m \n"
fi

echo "Testing: GET /cantRead HTTP/1.0"
echo -e -n "GET /cantRead HTTP/1.0\r\n\r\n" | nc -u -w1 127.0.0.1 8080 > tmp
if diff tmp ./test-out/cantRead > /dev/null; then
  printf "  \e[32m Pass! \e[0m \n"
else
  printf "  \e[31m Fail! \e[0m \n"
fi

echo "Testing: GET /runme1st.sh HTTP/1.0"
echo -e -n "GET /runme1st.sh HTTP/1.0\r\n\r\n" | nc -u -w1 127.0.0.1 8080 > tmp
if diff tmp ./test-out/runme1st.sh > /dev/null; then
  printf "  \e[32m Pass! \e[0m \n"
else
  printf "  \e[31m Fail! \e[0m \n"
fi

echo "Testing: GET /gnu/main.html HTTP/1.0"
echo -e -n "GET /gnu/main.html HTTP/1.0\r\n\r\n" | nc -u -w1 127.0.0.1 8080 > tmp
if diff tmp ./test-out/gnu.html > /dev/null; then
  printf "  \e[32m Pass! \e[0m \n"
else
  printf "  \e[31m Fail! \e[0m \n"
fi

echo "Testing: GET /herpderp HTTP/1.0"
echo -e -n "GET /herpderp HTTP/1.0\r\n\r\n" | nc -u -w1 127.0.0.1 8080 > tmp
if diff tmp ./test-out/herpderp > /dev/null; then
  printf "  \e[32m Pass! \e[0m \n"
else
  printf "  \e[31m Fail! \e[0m \n"
fi

echo "Testing: GET /../ HTTP/1.0"
echo -e -n "GET /../ HTTP/1.0\r\n\r\n" | nc -u -w1 127.0.0.1 8080 > tmp
if diff tmp ./test-out/dot_dot > /dev/null; then
  printf "  \e[32m Pass! \e[0m \n"
else
  printf "  \e[32m Fail! \e[0m \n"
fi

echo "Testing: GET . HTTP/1.0"
echo -e -n "GET . HTTP/1.0\r\n\r\n" | nc -u -w1 127.0.0.1 8080 > tmp
if diff tmp ./test-out/dot_dot > /dev/null; then
  printf "  \e[32m Pass! \e[0m \n"
else
  printf "  \e[31m Fail! \e[0m \n"
fi

# Clean up
rm tmp
