<?php

$ip = "127.0.0.1";
$port = 55555;
$str = $argv[1]; 



$sock = socket_create(AF_INET, SOCK_DGRAM, SOL_UDP); 
socket_set_option($sock, SOL_SOCKET, SO_BROADCAST, 1);
socket_set_option($sock, SOL_SOCKET, SO_RCVTIMEO, array("sec"=>5, "usec"=>0));
socket_sendto($sock, $str, strlen($str), 0, $ip, $port);
echo "\r\n--------------------\r\nSending: \r\n".$str."\r\n";

echo "Hex: \r\n".bin2hex($str)."\r\n\r\n";
while(true) {
  $ret = @socket_recvfrom($sock, $buf, 2048, 0, $ip, $port);
  if($ret === false) break;

  echo "Receiving: \r\n" .$buf."\r\n";

  echo "Hex: \r\n" .bin2hex($buf)."\r\n\r\n";

  
}

echo "\r\nSent Bytes:". strlen($str)."\r\nReceived Bytes:".strlen($buf)."\r\n";

socket_close($sock);


?>