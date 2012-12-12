## Building
* General Use:
  * Install [Qt](http://qt.digia.com/Product/)
  * Install [Qt Cryptographic Architecture
    library](http://delta.affinix.com/docs/qca/index.html)
* Sample application (see below)

---
## Kademlia Client
An implementation of a distributed hash table (DHT) using the
[Kademlia](http://pdos.csail.mit.edu/~petar/papers/maymounkov-kademlia-lncs.pdf)
algorithm.
#### API
For programs wishing to use a DHT to store files across a local area
network (LAN), create a KademliaClientThread, which provides
asynchronouse Find, Store, and Remove functionality through use of the following
methods:

* QIODevice* Find(Qkey key)
* void Store(QKey key, QIODevice* device)
* void Remove(QKey key)

In order to make a synchronous find request, create a local QEventLoop and
connect its quit() slot to the KademliaClientThread's FindRequestComplete()
signal.  The found value can be retrieved with the accessor,
get_last_found_value(), which returns a pointer to the value coresponding to the
last lookup, or NULL if the resource could not be found in the DHT.

#### Message Protocol
A basic packet contains the following fields:

* NodeId: of sending node
* RequestId: unique identifier for the request (not globally unique)
* Type: of request/reply

In addition, a packet might contain the following keys:

* Id (FIND_NODE): of requested node
* Key (FIND_VALUE): of requested resource
* Nodes (FIND_NODE_REPLY): of k-nodes with ids closest to requested
  resource (node or value)
* Key (STORE, FIND_VALUE_REPLY): of resource being stored/requested

The Type field may take one of the following values:

* JOIN, JOIN_REPLY
* PING, ACK
* STORE, READY_DOWNLOAD
* FIND_NODE, FIND_NODE_REPLY
* FIND_VALUE, FIND_VALUE_REPLY

The JOIN pair is an addition to the protocol that reflects the target
use case for the client implementation, i.e. a LAN. Instead of invoking a
FIND_NODE as specified by the original protocol, each client sends a broadcast
to the LAN with its Node Id. All clients then respond with their own Node Ids,
and the bootstrapping node fills its k-buckets with their responses.

READY_DOWNLOAD is simply an acknowledgement of the STORE RPC, and
signifies that the target client is preparing to open a TCP connection
with the sending client in order to transfer the given resource.

---
## Peer Cache
A subclass of QAbstractNetworkCache that can be used with a
QNetworkAccessManager to cache network requests across a LAN. Built
using the Kademlia client described above; a PeerCache object
communicates with a KademliaClientThread through signals and slots,
making synchronous requests to the client by creating a local
QEventLoop.
#### API
For use as part of network application, see the documentation for
[QAbstractNetworkCache](http://doc.qt.digia.com/qt/qabstractnetworkcache.html)
#### Stand-Alone Use
To use as a basic interface for making HTTP Get requests, invoke the
built program on the command line as follows:

    cd src
    qmake
    make
    peer_cache
Enter the requested url in the text box and hit enter to make a request.
To switch into Cache-only mode (i.e. offline), click the button below
the text box.
