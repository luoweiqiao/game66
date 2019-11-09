
find ./cs_msg -name "*.proto" | xargs protoc -I=./cs_msg/ --cpp_out=./cs_msg/cpp/

cp ./cs_msg/cpp/*.*   ../server/servers/common/src/pb
