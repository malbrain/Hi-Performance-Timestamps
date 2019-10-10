# Hi-Performance-Timestamps
Transaction Commitment Timestamp generator for a thousand cores

The basic idea is to create and deliver timestamps for client transactions on the order of a billion timestamp requests per second.  The generator must have an extremely short execution path that doesn't require atomic operations.
