package main

import (
    "fmt"
    "net"
    "strconv"
    "strings"
    "os"
    "bufio"
   // "strconv"
    "unsafe"
    "encoding/binary"
    "time"
)

func CheckError(err error) {
    if err  != nil {
        fmt.Println("Error: " , err)
    }
}

func main() {
  //fmt.Println("client started")

  ServerAddr, err := net.ResolveUDPAddr("udp", "137.148.205.255:21574")
  CheckError(err);

  LocalAddr, err := net.ResolveUDPAddr("udp", ":0")
  CheckError(err)

  //CLNS, err := net.DialUDP("udp", LocalAddr, ServerAddr)
//MAJOR BUG: i had listenUDP with serveraddr and receiving "getcisbank" as
  //the packet because im listening on the UDP socket on the sevrer
  //we change the serveraddr to localaddr and it works
  CLNS, err := net.ListenUDP("udp", LocalAddr)
  CheckError(err)
  defer CLNS.Close()
  msg := []byte("GET CISBANK")
  //lilend := make([]byte, 1024)
  //can't use := here because no new varibles left of :=

  //bug here because it was write() + dialUDP, since we use listenUDP, here we can't use write()
    //and we have to use writeto or writetoUDP
  _, err = CLNS.WriteToUDP(msg, ServerAddr)
  //_, err = CLNS.Write(msg)
  CheckError(err)

  //get the IP and Port number back
  //BUG: client can't read message, it's blocking
  //###############dialUDP means client can't receive packets, use listenUDP to connect
  //n, addr, err := CLNS.ReadFromUDP(buf2)
  buf2 := make([]byte, 4096)
  _, _, err = CLNS.ReadFromUDP(buf2)
  //_, err = bufio.NewReader(CLNS).Read(buf2)
  CheckError(err)

//we need to convert buf2 from Big Endian to Little Endian order if we want to see it
  //fmt.Println(buf2)
  //fmt.Println(string(buf2))


  //fmt.Println(n, " ", addr.IP.String())
  buf3 := strings.Split(strings.TrimRight(string(buf2), "\n"), ":")
  if len(buf3) == 1 {
    fmt.Println("invalid answer from servicemap, can't split on :")
    os.Exit(1)
  }
  fmt.Println("Service provided by ", buf3[0], " at port ", buf3[1])
  x := -1 //flag where 0 is query, 1 is update

  //tcp with server-------------------------------------------------------------
  var ask uint32 = 0
  reader := bufio.NewReader(os.Stdin)
  fmt.Print("Enter your request: ")
  text, _ := reader.ReadString('\n')
  text = text[:len(text)-1]
  tokens := strings.Split(text, " ")
  askbuf := make([]byte,4)
  conn, err := net.Dial("tcp", string(buf2)) //so dial takes string
  CheckError(err)
  for tokens[0] != "quit" {//error has ord1, ord2, and ord3 are undefined so added this (BUG)
	  if tokens[0] == "query" {
		x = 0
		ask = 1000
		binary.BigEndian.PutUint32(askbuf,ask)
		//ord1 = BigEndian(tokens[1])
		//fmt.Println(ord1)
	  }else if tokens[0] == "update" {
		if len(tokens) < 3 {
		  fmt.Println("please enter 3 tokens for update");
		  fmt.Print("Enter your request: ")
    	  text, _ = reader.ReadString('\n')
    	  text = text[:len(text)-1]
    	  tokens = strings.Split(text, " ")
    	  askbuf = make([]byte,4)
    	  continue
		}
		x = 1
		ask = 1001
		binary.BigEndian.PutUint32(askbuf,ask)
		//ord2 = BigEndian(tokens[1])
		//ord3 = BigEndian(tokens[2])
		//fmt.Println(ord2, ord3)
	  }else {
		x=3
		fmt.Println("please enter either query or update and acct num separated by a space")
		fmt.Print("Enter your request: ")
    	text, _ = reader.ReadString('\n')
    	text = text[:len(text)-1]
    	tokens = strings.Split(text, " ")
    	askbuf = make([]byte,4)
    	continue
	  }
	  acctint,_ := strconv.Atoi(tokens[1])
	  anum := uint32(acctint)
	  //fmt.Println(acctint)
	  abuf := make([]byte,4)
	  binary.BigEndian.PutUint32(abuf,anum)

	  if x == 0 {
		conn.Write(askbuf)
		//bug:everything gettins written at once (1000 plus the acct num)
		time.Sleep(250000000)
		conn.Write(abuf)
	  }else if x ==1 { //x is 1
		 of,_ := strconv.ParseFloat(tokens[2],32)
		 f := float32(of)
		 var ip *uint32
		 ip = (*uint32)(unsafe.Pointer(&f))
		 vbuf := make([]byte,4)
		 binary.BigEndian.PutUint32(vbuf,*ip)

		conn.Write(askbuf)
		time.Sleep(250000000)
		conn.Write(abuf)
		time.Sleep(250000000)
		conn.Write(vbuf)
	  }
	  if x != 3{
	    buffer := make([]byte, 1024)
	    n,_ := conn.Read(buffer)
	    fmt.Printf("%s\n", buffer[:n])
	  }
    fmt.Print("Enter your request: ")
    text, _ = reader.ReadString('\n')
    text = text[:len(text)-1]
    tokens = strings.Split(text, " ")
    askbuf = make([]byte,4)
  }
	conn.Close()
}
