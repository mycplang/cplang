// test_websocket.cp - WebSocket smoke test
// Tests function availability and nil guards (no server needed)

a = wsConnect("not_a_url")
打印(isNil(a))

b = wsClose(a)
打印(b)

c = wsSend(a, "hello")
打印(c)

d = wsIsOpen(a)
打印(d)

e = wsRecv(a)
打印(isNil(e))

打印("WS_SMOKE_PASS")
