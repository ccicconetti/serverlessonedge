# Integration of serverless edge computing and Apache OpenWhisk

It is enabled thanks to two dedicated components:

- `wskproxy`: Acts as an HTTP proxy and translates action invocations into edge lambda function requests through gRPC calls; every proxy is bound to a given edge server (can be an edge router or an edge computer).
- `edgecomputerwsk`: Receives incoming lambda request functions and forwards them to an OpenWhisk server, whose API host URL and authentication token must be known at launch time.

In any case, the edge routers forward lambda functions calls without any notion on whether they are eventually intended for an OpenWhisk server or not.

The control plane, as managed by the edge controller, is also totally transparent to the OpenWhisk components: actions are advertised, complete with their namespace, as if they were lambda functions executed locally by an edge computer.

A mix of OpenWhisk and non-OpenWhisk traffic of lambda functions is possible.


## Example

Below there is a diagram of an example scenario that can be set up to allow a single OpenWhisk client to invoke actions from two OpenWhisk servers through a serverless edge domain.

![](openwhisk_integration.png)

### Assumptions

In the rest of the example we assume that:

- all serverless edge components are executed on the same host (not required) 
- that host can reach two OpenWhisk servers at known URLs and with proper credentials
- the host running the OpenWhisk client (`wsk`) can also reach both OpenWhisk servers (only required for the set up)
- the two OpenWhisk servers can be reached at `OW1` (or `OW2`) with given authentication token `AUTH1` (or `AUTH2`)

### Preliminary operations

Create an action called `copy` on both OpenWhisk servers, by executing the following commands on the client host:

```
cat > copy.js << EOF
function main (params) {
  var input = params.input || ''
  return { payload: input }
}
EOF
wsk action create copy copy.js \
	--apihost $OW1 \
	--auth $AUTH1
wsk action create copy copy.js \
	--apihost $OW2 \
	--auth $AUTH2
```

Check that you can execute the action on both servers:

```
wsk action invoke /guest/copy \
	-b -r -p input hello \
	--apihost $OW1 \
	--auth $AUTH1
wsk action invoke /guest/copy \
	-b -r -p input hello \
	--apihost $OW2 \
	--auth $AUTH2
```

will both return:

```
{
    "payload": "hello"
}
```

### Setup

Open five shells and execute the following commands from the `Executables/` directory after having built the serverless edge computing source code.

Start the edge controller, which will listen automatically to `0.0.0.0:6475`:

```
./edgecontroller
```

Start the edge router, which will listen automatically to `0.0.0.0:6473`, by specifying the edge controller end-point:

```
./edgerouter --controller 127.0.0.1:6475
```

Start the two OpenWhisk edge computers, telling them to listen to `127.0.0.1:10000` (or `127.0.0.1:10001`) for incoming lambda function requests, and to forward them to the respective OpenWhisk servers:

```
./edgecomputerwsk \
	--wsk-api-root $OW1 \
	--wsk-auth $AUTH1 \
	--controller 127.0.0.1:6475 \
	--server 127.0.0.1:10000
```

```
./edgecomputerwsk \
	--wsk-api-root $OW2 \
	--wsk-auth $AUTH2 \
	--controller 127.0.0.1:6475 \
	--server 127.0.0.1:10000
```

Finally, start the OpenWhisk client proxy so that it exposes to clients an API at `http://127.0.0.1:20000` and connects to the edge router at `127.0.0.1:6473`:

```
./wskproxy \
	--wsk-api-root http://127.0.0.1:20000 \
	--server 127.0.0.1:6473
```

### Execution

Invoke the action `copy` from OpenWhisk client connecting to the proxy above:

```
wsk action invoke /guest/copy \
	-b -r -p input hello \
	--apihost $OW1 \
	--auth $AUTH1
```

You will obtain the same result as if calling one of the OpenWhisk servers directly, i.e.:

```
{
    "payload": "hello"
}
```

The edge router will forward the invocation to different OpenWhisk servers over multiple calls. This can be verified (e.g.) by restarting the command `wskproxy` while increasing the logging verbosity by setting the environment variable `GLOG_v=1`.

Possible output:

```
I0206 15:47:00.874366 277393408 server.cpp:52] REST server started at http://127.0.0.1:20000/
I0206 15:47:02.312391 257003520 server.cpp:68] POST /api/v1/namespaces/guest/actions/copy?blocking=true&result=true
I0206 15:47:02.561408 257003520 wskproxy.cpp:88] 0.248579 retcode: OK, from: OW1, ptime: 0 ms, hops: 3, load: 0/0/0, output: hello, dataout size: 0
I0206 15:49:58.803352 260222976 server.cpp:68] POST /api/v1/namespaces/guest/actions/copy?blocking=true&result=true
I0206 15:49:59.025393 260222976 wskproxy.cpp:88] 0.221772 retcode: OK, from: OW1, ptime: 0 ms, hops: 3, load: 0/0/0, output: hello, dataout size: 0
I0206 15:50:01.968039 263979008 server.cpp:68] POST /api/v1/namespaces/guest/actions/copy?blocking=true&result=true
I0206 15:50:02.146775 263979008 wskproxy.cpp:88] 0.178491 retcode: OK, from: OW2, ptime: 0 ms, hops: 3, load: 0/0/0, output: hello, dataout size: 0
I0206 15:50:03.830055 267735040 server.cpp:68] POST /api/v1/namespaces/guest/actions/copy?blocking=true&result=true
I0206 15:50:03.997776 267735040 wskproxy.cpp:88] 0.167468 retcode: OK, from: OW1, ptime: 0 ms, hops: 3, load: 0/0/0, output: hello, dataout size: 0
```

## Security considerations

Currently the `wskproxy` accepts all invocations, which are authorized based on the credentials made available to the `edgecomputerwsk`. This is inherently insecure. The current release is intended only for experimentation in a closed and trusted environment.

## Limitations

`wskclient` only allows to invoke actions (no listing or any other command is supported), and only with the `blocking` and `result` flags true.