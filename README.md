##### A Distributed Hash Table-Based Protocol for Improving Bandwidth Efficiency on Ad-Hoc, Local Area Networks

  
*Daniel Tahara, Yale University*


###### Overview: 
Despite the ubiquity of ad-hoc, Local Area Networks (LAN) in both homes and public settings, very little work improvements have been made to the basic structure of single access point with multiple nodes independently connected to that access point.  The current network topology places a huge burden on the access point (generally a router), because it lacks coordination among the individual nodes.  As a result, the router faces a disproportionate amount of traffic as it attempts to handle the simultaneous (and often duplicate) resource requests originating from the LAN, and the resulting congestion causes higher latency and the potential for packet loss.  My project aims to create the basic solution to the problem of network congestion and bandwidth limitations of LANs by using a distributed hash table (DHT) to both structure the network topology and cache web requests.  By routing all resource requests first through a DHT, frequently accessed web content can be served to users with lower latency and in a way that minimizes bandwidth. 

###### Protocol:
The basic protocol includes a Kademlia-based DHT, with a low replication factor in order to ensure that content updates frequently (will roughly correlate with network churn).  The exact replication factor and expiration can be more finely tuned and vary based on content type, but that is beyond the scope of my project. 

The general structure of the system will be as follows:

* Kademlia-based DHT
	* Protocol Messages
		* PING - see if a node is still alive
		* STORE - store a key, value pair, where the key is the SHA-256 hash of the content's URI and the value is the actual content
		* FIND_NODE - used by clients to join the network and locate nodes for its routing table
		* FIND_VALUE - retrieve cached content corresponding to a key
	* Routing Tables
		* Define distance as the XOR of the two node IDs
		* Each node will have n buckets of size k (where the size of the hash table is 2^n).  In bucket 0 <= m <= m, each node will store a list of k other nodes whose distance has a most significant bit of 1 in the m-th digit
		* As nodes search for content, the routing tables will be updated on the basis of responses of other nodes
	* Joining
		* Compute random node ID
		* Submit search for that node ID, using responses to fill routing tables
* Web requests
	* Route first through the DHT
	* If content cannot be found, responsible node downloads content, stores locally, and forwards to requesting node

Since the DHT is to be used as a web cache, my implementation of a DHT will differ from the Kademlia algorithm in the case that a given key cannot be found in the table (i.e. on a  FIND_VALUE).  In such an event, the node responsible for that value (on the basis of its key) will submit and HTTP GET request for that URI, and, upon receiving and storing the content locally, forward it to the original requesting node. 

###### Extensions:

Potential extensions or enhancements to the protocol might include storing content in fixed or variable-length blocks, where the key is the SHA-256 hash of the content itself instead of a hash of the URI.  The hash IDs can then be indexed and distributed with a basic rumor-mongering protocol or other gossip mechanism.  Furthermore, in order to improve caching and minimize 'hot-spots' for frequently requested content, individual nodes might also store and index content that they themselves have requested, responding directly to subsequent requests for that content rather than forwarding the request along.  The relaxation of the DHT protocol is similar to that employed by CoralCDN, and would similarly help distribute load and reduce latency of requests.

###### Testing:
The system may be tested for basic functionality by ensuring that each request is first routed through the local DHT.  When multiple nodes request identical content, we can verify that the second (and subsequent) requests are fulfilled by nodes in the LAN.  The system's performance might be measured by comparing bandwidth usage and network traffic on LANs both with and without DHT-caching, as well as calculating the average latency of requests before and after the protocol has been implemented.

###### Limitations:
The above solution is most likely to see performance gains in a more structured network such as a corporate network, where a network administrator might be able to impose additional constraints on the network topology and use tools such as OpenFlow to separate the logic of routing from the actual data transfer.  Direct links (via network switches, for example) between pairs of nodes could also minimize network traffic, with query packets sent directly to the recipient node rather than going first through the router.
