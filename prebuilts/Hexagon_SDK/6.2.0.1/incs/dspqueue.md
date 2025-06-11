# Asynchronous DSP Packet Queue

## API Overview {#api-overview}

The Asynchronous DSP Packet Queue is accessed through a simple C
API. Most of the API is identical on both the host CPU and DSP with
the exception that queues can only be created on the CPU.

* dspqueue_create(): Create a new queue. Queues can only be created
  on the host CPU.

* dspqueue_close(): Close a queue

* dspqueue_export(): Export a queue on the host CPU, creating a
  handle that be used with dspqueue_import() on the DSP.

* dspqueue_import(): Import a queue for use on the DSP, using a
  handle returned from dspqueue_export() on the CPU.

* dspqueue_write() / dspqueue_write_noblock(): Write a packet to a
  queue. Writes can either block if the queue is full or return an
  error (dspqueue_write_noblock()); blocking writes can optionally
  have a timeout.

* dspqueue_read() / dspqueue_read_noblock(): Read a packet from a
  queue.

* dspqueue_peek() / dspqueue_peek_noblock(): Retrieve information
  about the next packet without consuming it.

* dspqueue_write_early_wakeup_noblock(): Write an early wakeup packet to the
  queue.

* dspqueue_get_stat(): Retrieve queue statistics, including the number
  of packets queued and statistics about early wakeup.

