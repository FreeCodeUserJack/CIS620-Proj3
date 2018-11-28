package main

import (
  "fmt"
  "net"
  "os"
  "strings"
  //"reflect"
)

func CheckError(err error) {
  if err != nil {
    fmt.Println("Error: ", err)
    os.Exit(0)
  }
}

func main() {
  //fmt.Println("starting servicemap")

  var table map[string]string
  //bug: couldn't assign anything to table because it hasn't been initialized yet (no memory space)
  table = make(map[string]string)

  //table["CISBANK"] = "137.148.205.125:54754"
/*
  for k, v := range table {
    fmt.Println(k)
    fmt.Println(v)
  }
*/
//############the value 137.148.205.125:54754 when turned to a byte slice, each digit
  //will be turned into its ascii value, . is 46, 1 is 49
  //21574 is hardcoded port assigned by the proj guidelines
  ServerAddr, err := net.ResolveUDPAddr("udp", ":21574")
  CheckError(err);

  ServerCLNS, err := net.ListenUDP("udp", ServerAddr)
  CheckError(err);
  defer ServerCLNS.Close()

  buf := make([]byte, 1024)
  buf3 := ""
  var buf4 string = ""
  var flag int
  for { //need a loop here to keep reading
    //fmt.Println("ready for service request")
    n,addr,err := ServerCLNS.ReadFromUDP(buf)
    //buf2 := string(addr.IP) //is buf2 not used here??
    fmt.Println("Received from ", addr.IP.String(), ":", string(buf[0:n]))
    CheckError(err)

    input := string(buf)
    strbuf := strings.Split(input, " ")
    //fmt.Println(strbuf[0], " ", strbuf[1])
    //buf4 := ""
    if strbuf[0] == "PUT" {
      //bug: remember to call IP class String() function to turn IP address to string
      buf3 = string(addr.IP.String())
      //fmt.Println(buf3)
      buf3 += ":"
      //fmt.Println(buf3)
      buf3 += strbuf[2]
      table[strbuf[1]] = buf3
      //if i keep := then it says "no new variables to the left of :="
      _,err = ServerCLNS.WriteToUDP([]byte("OK"), addr)
      CheckError(err)
/*
      for k, v :=range table {
        fmt.Println("registered ", k, " with ", v)
      }
*/
    } else if strbuf[0] == "GET" {
      flag = 0
      for k, v := range table {
        //fmt.Println(k, " ", v) //bug: loop is entered but the if statement doesn't work
        //fmt.Println(k, strbuf[1])
        //fmt.Println(k == strbuf[1][0:len(k)]) //!!!!!why are they not equal?
          //fmt.Println(strbuf[1][0:len(k)])  //test to see if 0"len(k) is right
        //fmt.Println(reflect.TypeOf(k), reflect.TypeOf(strbuf[1])) //both are string types
        //fmt.Println(len(k), len(strbuf[1])) //BUG THEY AREN"T THE SAME LENGTH
          //one is 7 and the other is 1020
        if k == strbuf[1][0:len(k)] {
          //fmt.Println(v)
          buf4 = v
          flag = 1
          break
        }
        //fmt.Println(k, v) //this doesn't get printed
      }
      //fmt.Println(buf4) //// BUG: buf2 is undefined but now it's fine because it was junk value

      //now to turn buf2, a string, into a slice output
      if flag == 1 {
        buf = []byte(buf4)
      } else {
        buf = []byte("service not found")
      }
      //fmt.Println(b) //prints nonsense
      //fmt.Println(string(b))
      //fmt.Println(addr.IP.String())
      //fmt.Println(string(buf))
      _,err = ServerCLNS.WriteToUDP(buf, addr) //it's a successful write back
      //BUG: client cannot receive the information from servicemap
      CheckError(err)

      //else has to be right after the closing brace or else error
    } else {
      _,err = ServerCLNS.WriteToUDP([]byte("BAD"), addr)
      CheckError(err)
    }
    //fmt.Println("service request finished")
  }
}
