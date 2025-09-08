/*
 * Copyright (c) 2023-2024 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#ifndef NCCL_OFI_OFIRAII_H_
#define NCCL_OFI_OFIRAII_H_

#include <memory>
#include <stdexcept>
#include <cstdint>

#include <rdma/fabric.h>
#include <rdma/fi_cm.h>
#include <rdma/fi_domain.h>
#include <rdma/fi_endpoint.h>
#include <rdma/fi_tagged.h>
#include <rdma/fi_rma.h>

/**
 * @brief RAII wrapper for Libfabric Fabric (fid_fabric)
 *
 * This class provides automatic resource management for Libfabric fabrics,
 * ensuring proper cleanup and exception safety. The wrapper is designed to be used
 * with shared_ptr for lifetime management and supports move semantics.
 *
 * Key design principles:
 * - RAII compliance: Constructor acquires resource, destructor releases it
 * - Move-only semantics: Prevents accidental copying and double-close
 * - No dependencies: Fabric is the root resource in the dependency chain
 * - Exception safety: Constructor throws on failure, destructor is noexcept
 * - Resource isolation: Only manages fabric, not other Libfabric resources
 */
class ofi_fabric_raii {
	private:
	struct fid_fabric *fabric_;

	public:
	/**
	 * @brief Constructor - creates and opens a fabric
	 *
	 * @param fabric_attr Libfabric fabric attributes for creation
	 * @throws std::runtime_error if fi_fabric fails
	 */
	explicit ofi_fabric_raii(struct fi_fabric_attr *fabric_attr);

	/**
	 * @brief Move constructor
	 *
	 * Transfers ownership of the fabric resource from other to this instance.
	 * The other instance will be left in a valid but empty state.
	 *
	 * @param other Source object to move from
	 */
	ofi_fabric_raii(ofi_fabric_raii &&other) noexcept;

	/**
	 * @brief Move assignment operator
	 *
	 * Transfers ownership of the fabric resource from other to this instance.
	 * If this instance currently owns a fabric, it will be properly closed first.
	 *
	 * @param other Source object to move from
	 * @return Reference to this instance
	 */
	ofi_fabric_raii &operator=(ofi_fabric_raii &&other) noexcept;

	/**
	 * @brief Destructor - automatically closes the fabric
	 *
	 * Calls fi_close on the underlying fid_fabric if it's valid.
	 * This destructor is noexcept to ensure proper RAII behavior.
	 */
	~ofi_fabric_raii() noexcept;

	// Delete copy constructor and copy assignment to prevent double-close
	ofi_fabric_raii(const ofi_fabric_raii &) = delete;
	ofi_fabric_raii &operator=(const ofi_fabric_raii &) = delete;

	/**
	 * @brief Get direct access to the underlying fid_fabric
	 *
	 * Provides access to the raw Libfabric fabric handle for compatibility
	 * with existing code that expects fid_fabric*.
	 *
	 * @return Pointer to the underlying fid_fabric, or nullptr if invalid
	 */
	struct fid_fabric *get() const noexcept;

	/**
	 * @brief Arrow operator for direct access to fid_fabric members
	 *
	 * Allows direct access to fid_fabric members using -> syntax.
	 *
	 * @return Pointer to the underlying fid_fabric
	 */
	struct fid_fabric *operator->() const noexcept;

	/**
	 * @brief Dereference operator for direct access to fid_fabric
	 *
	 * Allows direct access to the fid_fabric object using * syntax.
	 *
	 * @return Reference to the underlying fid_fabric
	 */
	struct fid_fabric &operator*() const noexcept;

	/**
	 * @brief Check if the fabric is valid
	 *
	 * @return true if the fabric is valid and can be used, false otherwise
	 */
	bool is_valid() const noexcept;

};

/**
 * @brief RAII wrapper for Libfabric Domain (fid_domain)
 *
 * This class provides automatic resource management for Libfabric domains,
 * ensuring proper cleanup and exception safety. The wrapper is designed to be used
 * with shared_ptr for lifetime management and supports move semantics.
 *
 * Key design principles:
 * - RAII compliance: Constructor acquires resource, destructor releases it
 * - Move-only semantics: Prevents accidental copying and double-close
 * - Dependency injection: Takes fabric wrapper as shared_ptr parameter
 * - Exception safety: Constructor throws on failure, destructor is noexcept
 * - Resource isolation: Only manages domain, not other Libfabric resources
 */
class ofi_domain_raii {
	private:
	struct fid_domain *domain_;

	public:
	/**
	 * @brief Constructor - creates and opens a domain
	 *
	 * @param fabric Shared pointer to fabric wrapper (ensures fabric lifetime)
	 * @param info Libfabric info structure for domain creation
	 * @throws std::runtime_error if fi_domain fails
	 */
	explicit ofi_domain_raii(std::shared_ptr<ofi_fabric_raii> fabric,
				struct fi_info *info);

	/**
	 * @brief Move constructor
	 *
	 * Transfers ownership of the domain resource from other to this instance.
	 * The other instance will be left in a valid but empty state.
	 *
	 * @param other Source object to move from
	 */
	ofi_domain_raii(ofi_domain_raii &&other) noexcept;

	/**
	 * @brief Move assignment operator
	 *
	 * Transfers ownership of the domain resource from other to this instance.
	 * If this instance currently owns a domain, it will be properly closed first.
	 *
	 * @param other Source object to move from
	 * @return Reference to this instance
	 */
	ofi_domain_raii &operator=(ofi_domain_raii &&other) noexcept;

	/**
	 * @brief Destructor - automatically closes the domain
	 *
	 * Calls fi_close on the underlying fid_domain if it's valid.
	 * This destructor is noexcept to ensure proper RAII behavior.
	 */
	~ofi_domain_raii() noexcept;

	// Delete copy constructor and copy assignment to prevent double-close
	ofi_domain_raii(const ofi_domain_raii &) = delete;
	ofi_domain_raii &operator=(const ofi_domain_raii &) = delete;

	/**
	 * @brief Get direct access to the underlying fid_domain
	 *
	 * Provides access to the raw Libfabric domain handle for compatibility
	 * with existing code that expects fid_domain*.
	 *
	 * @return Pointer to the underlying fid_domain, or nullptr if invalid
	 */
	struct fid_domain *get() const noexcept;

	/**
	 * @brief Arrow operator for direct access to fid_domain members
	 *
	 * Allows direct access to fid_domain members using -> syntax.
	 *
	 * @return Pointer to the underlying fid_domain
	 */
	struct fid_domain *operator->() const noexcept;

	/**
	 * @brief Dereference operator for direct access to fid_domain
	 *
	 * Allows direct access to the fid_domain object using * syntax.
	 *
	 * @return Reference to the underlying fid_domain
	 */
	struct fid_domain &operator*() const noexcept;

	/**
	 * @brief Check if the domain is valid
	 *
	 * @return true if the domain is valid and can be used, false otherwise
	 */
	bool is_valid() const noexcept;

};

/**
 * @brief RAII wrapper for Libfabric Completion Queue (fid_cq)
 *
 * This class provides automatic resource management for Libfabric completion queues,
 * ensuring proper cleanup and exception safety. The wrapper is designed to be used
 * with shared_ptr for lifetime management and supports move semantics.
 *
 * Key design principles:
 * - RAII compliance: Constructor acquires resource, destructor releases it
 * - Move-only semantics: Prevents accidental copying and double-close
 * - Dependency injection: Takes domain wrapper as shared_ptr parameter
 * - Exception safety: Constructor throws on failure, destructor is noexcept
 * - Resource isolation: Only manages CQ, not other Libfabric resources
 */
class ofi_cq_raii {
	private:
	struct fid_cq *cq_;

	public:
	/**
	 * @brief Constructor - creates and opens a completion queue
	 *
	 * @param domain Shared pointer to domain wrapper (ensures domain lifetime)
	 * @param attr CQ attributes for creation
	 * @throws std::runtime_error if fi_cq_open fails
	 */
	explicit ofi_cq_raii(std::shared_ptr<ofi_domain_raii> domain,
			     struct fi_cq_attr &attr);

	/**
	 * @brief Move constructor
	 *
	 * Transfers ownership of the CQ resource from other to this instance.
	 * The other instance will be left in a valid but empty state.
	 *
	 * @param other Source object to move from
	 */
	ofi_cq_raii(ofi_cq_raii &&other) noexcept;

	/**
	 * @brief Move assignment operator
	 *
	 * Transfers ownership of the CQ resource from other to this instance.
	 * If this instance currently owns a CQ, it will be properly closed first.
	 *
	 * @param other Source object to move from
	 * @return Reference to this instance
	 */
	ofi_cq_raii &operator=(ofi_cq_raii &&other) noexcept;

	/**
	 * @brief Destructor - automatically closes the completion queue
	 *
	 * Calls fi_close on the underlying fid_cq if it's valid.
	 * This destructor is noexcept to ensure proper RAII behavior.
	 */
	~ofi_cq_raii() noexcept;

	// Delete copy constructor and copy assignment to prevent double-close
	ofi_cq_raii(const ofi_cq_raii &) = delete;
	ofi_cq_raii &operator=(const ofi_cq_raii &) = delete;

	/**
	 * @brief Read completion entries from the queue
	 *
	 * Wrapper for fi_cq_read() that provides type safety and consistent interface.
	 *
	 * @param buf Buffer to store completion entries
	 * @param count Maximum number of entries to read
	 * @return Number of entries read on success, negative error code on failure
	 */
	ssize_t read(void *buf, size_t count) const;

	/**
	 * @brief Read error entries from the queue
	 *
	 * Wrapper for fi_cq_readerr() that provides type safety and consistent interface.
	 *
	 * @param buf Buffer to store error entry
	 * @param flags Flags for the operation (typically 0)
	 * @return 1 on success (error entry read), negative error code on failure
	 */
	ssize_t read_error(struct fi_cq_err_entry *buf, uint64_t flags) const;

	/**
	 * @brief Convert provider error to human-readable string
	 *
	 * Wrapper for fi_cq_strerror() that provides type safety and consistent interface.
	 *
	 * @param prov_errno Provider-specific error number
	 * @param err_data Provider-specific error data
	 * @param buf Buffer to store error string (can be nullptr)
	 * @param len Length of buffer
	 * @return Pointer to error string
	 */
	const char *strerror(int prov_errno, const void *err_data,
			     char *buf, size_t len) const;

	/**
	 * @brief Get direct access to the underlying fid_cq
	 *
	 * Provides access to the raw Libfabric CQ handle for compatibility
	 * with existing code that expects fid_cq*.
	 *
	 * @return Pointer to the underlying fid_cq, or nullptr if invalid
	 */
	struct fid_cq *get() const noexcept;

	/**
	 * @brief Arrow operator for direct access to fid_cq members
	 *
	 * Allows direct access to fid_cq members using -> syntax.
	 *
	 * @return Pointer to the underlying fid_cq
	 */
	struct fid_cq *operator->() const noexcept;

	/**
	 * @brief Check if the CQ is valid
	 *
	 * @return true if the CQ is valid and can be used, false otherwise
	 */
	bool is_valid() const noexcept;

};

/**
 * @brief RAII wrapper for Libfabric Address Vector (fid_av)
 *
 * This class provides automatic resource management for Libfabric address vectors,
 * ensuring proper cleanup and exception safety. The wrapper is designed to be used
 * with shared_ptr for lifetime management and supports move semantics.
 *
 * Key design principles:
 * - RAII compliance: Constructor acquires resource, destructor releases it
 * - Move-only semantics: Prevents accidental copying and double-close
 * - Dependency injection: Takes domain wrapper as shared_ptr parameter
 * - Exception safety: Constructor throws on failure, destructor is noexcept
 * - Resource isolation: Only manages AV, not other Libfabric resources
 */
class ofi_av_raii {
	private:
	struct fid_av *av_;

	public:
	/**
	 * @brief Constructor - creates and opens an address vector
	 *
	 * @param domain Shared pointer to domain wrapper (ensures domain lifetime)
	 * @param attr AV attributes for creation
	 * @throws std::runtime_error if fi_av_open fails
	 */
	explicit ofi_av_raii(std::shared_ptr<ofi_domain_raii> domain,
			     struct fi_av_attr &attr);

	/**
	 * @brief Move constructor
	 *
	 * Transfers ownership of the AV resource from other to this instance.
	 * The other instance will be left in a valid but empty state.
	 *
	 * @param other Source object to move from
	 */
	ofi_av_raii(ofi_av_raii &&other) noexcept;

	/**
	 * @brief Move assignment operator
	 *
	 * Transfers ownership of the AV resource from other to this instance.
	 * If this instance currently owns an AV, it will be properly closed first.
	 *
	 * @param other Source object to move from
	 * @return Reference to this instance
	 */
	ofi_av_raii &operator=(ofi_av_raii &&other) noexcept;

	/**
	 * @brief Destructor - automatically closes the address vector
	 *
	 * Calls fi_close on the underlying fid_av if it's valid.
	 * This destructor is noexcept to ensure proper RAII behavior.
	 */
	~ofi_av_raii() noexcept;

	// Delete copy constructor and copy assignment to prevent double-close
	ofi_av_raii(const ofi_av_raii &) = delete;
	ofi_av_raii &operator=(const ofi_av_raii &) = delete;

	/**
	 * @brief Insert addresses into the address vector
	 *
	 * Wrapper for fi_av_insert() that provides type safety and consistent interface.
	 *
	 * @param addr Address(es) to insert
	 * @param count Number of addresses to insert
	 * @param fi_addr Buffer to store fabric addresses
	 * @param flags Flags for the operation
	 * @param context Context for asynchronous operations
	 * @return Number of addresses inserted on success, negative error code on failure
	 */
	int insert(const void *addr, size_t count, fi_addr_t *fi_addr, 
		   uint64_t flags, void *context) const;

	/**
	 * @brief Get direct access to the underlying fid_av
	 *
	 * Provides access to the raw Libfabric AV handle for compatibility
	 * with existing code that expects fid_av*.
	 *
	 * @return Pointer to the underlying fid_av, or nullptr if invalid
	 */
	struct fid_av *get() const noexcept;

	/**
	 * @brief Arrow operator for direct access to fid_av members
	 *
	 * Allows direct access to fid_av members using -> syntax.
	 *
	 * @return Pointer to the underlying fid_av
	 */
	struct fid_av *operator->() const noexcept;

	/**
	 * @brief Check if the AV is valid
	 *
	 * @return true if the AV is valid and can be used, false otherwise
	 */
	bool is_valid() const noexcept;

};

/**
 * @brief RAII wrapper for Libfabric Endpoint (fid_ep)
 *
 * This class provides automatic resource management for Libfabric endpoints,
 * ensuring proper cleanup and exception safety. The wrapper is designed to be used
 * with shared_ptr for lifetime management and supports move semantics.
 *
 * Key design principles:
 * - RAII compliance: Constructor acquires resource, destructor releases it
 * - Move-only semantics: Prevents accidental copying and double-close
 * - Dependency injection: Takes domain wrapper as shared_ptr parameter
 * - Exception safety: Constructor throws on failure, destructor is noexcept
 * - Resource isolation: Only manages endpoint, not other Libfabric resources
 */
class ofi_ep_raii {
	private:
	struct fid_ep *ep_;

	public:
	/**
	 * @brief Constructor - creates and opens an endpoint
	 *
	 * @param domain Shared pointer to domain wrapper (ensures domain lifetime)
	 * @param info Libfabric info structure for endpoint creation
	 * @throws std::runtime_error if fi_endpoint fails
	 */
	explicit ofi_ep_raii(std::shared_ptr<ofi_domain_raii> domain,
			     struct fi_info *info);

	/**
	 * @brief Move constructor
	 *
	 * Transfers ownership of the endpoint resource from other to this instance.
	 * The other instance will be left in a valid but empty state.
	 *
	 * @param other Source object to move from
	 */
	ofi_ep_raii(ofi_ep_raii &&other) noexcept;

	/**
	 * @brief Move assignment operator
	 *
	 * Transfers ownership of the endpoint resource from other to this instance.
	 * If this instance currently owns an endpoint, it will be properly closed first.
	 *
	 * @param other Source object to move from
	 * @return Reference to this instance
	 */
	ofi_ep_raii &operator=(ofi_ep_raii &&other) noexcept;

	/**
	 * @brief Destructor - automatically closes the endpoint
	 *
	 * Calls fi_close on the underlying fid_ep if it's valid.
	 * This destructor is noexcept to ensure proper RAII behavior.
	 */
	~ofi_ep_raii() noexcept;

	// Delete copy constructor and copy assignment to prevent double-close
	ofi_ep_raii(const ofi_ep_raii &) = delete;
	ofi_ep_raii &operator=(const ofi_ep_raii &) = delete;

	/**
	 * @brief Bind completion queue to endpoint
	 *
	 * Wrapper for fi_ep_bind() with CQ that provides type safety and consistent interface.
	 *
	 * @param cq Shared pointer to CQ wrapper to bind
	 * @param flags Binding flags (e.g., FI_TRANSMIT | FI_RECV)
	 * @return 0 on success, negative error code on failure
	 */
	int bind_cq(std::shared_ptr<ofi_cq_raii> cq, uint64_t flags);

	/**
	 * @brief Bind address vector to endpoint
	 *
	 * Wrapper for fi_ep_bind() with AV that provides type safety and consistent interface.
	 *
	 * @param av Shared pointer to AV wrapper to bind
	 * @param flags Binding flags (typically 0)
	 * @return 0 on success, negative error code on failure
	 */
	int bind_av(std::shared_ptr<ofi_av_raii> av, uint64_t flags);

	/**
	 * @brief Bind endpoint to a resource (CQ, AV, etc.)
	 *
	 * Wrapper for fi_ep_bind() that provides type safety and consistent interface.
	 *
	 * @param fid Resource to bind to endpoint
	 * @param flags Binding flags
	 * @return 0 on success, negative error code on failure
	 */
	int bind(struct fid *fid, uint64_t flags) const;

	/**
	 * @brief Enable endpoint for communication
	 *
	 * Wrapper for fi_enable() that provides type safety and consistent interface.
	 * Must be called after all necessary bindings are complete.
	 *
	 * @return 0 on success, negative error code on failure
	 */
	int enable() const;

	/**
	 * @brief Cancel outstanding operations
	 *
	 * Wrapper for fi_cancel() that provides type safety and consistent interface.
	 *
	 * @param context Context for operations to cancel
	 * @return 0 on success, negative error code on failure
	 */
	int cancel(void *context) const;

	/**
	 * @brief Set endpoint option
	 *
	 * Wrapper for fi_setopt() that provides type safety and consistent interface.
	 *
	 * @param level Option level (e.g., FI_OPT_ENDPOINT)
	 * @param optname Option name
	 * @param optval Option value
	 * @param optlen Option value length
	 * @return 0 on success, negative error code on failure
	 */
	int setopt(int level, int optname, const void *optval, size_t optlen) const;

	/**
	 * @brief Get endpoint option
	 *
	 * Wrapper for fi_getopt() that provides type safety and consistent interface.
	 *
	 * @param level Option level (e.g., FI_OPT_ENDPOINT)
	 * @param optname Option name
	 * @param optval Buffer to store option value
	 * @param optlen Pointer to option value length (input: buffer size, output: actual size)
	 * @return 0 on success, negative error code on failure
	 */
	int getopt(int level, int optname, void *optval, size_t *optlen) const;

	/**
	 * @brief Get endpoint name/address
	 *
	 * Wrapper for fi_getname() that provides type safety and consistent interface.
	 *
	 * @param addr Buffer to store endpoint address
	 * @param addrlen Pointer to address length (input: buffer size, output: actual size)
	 * @return 0 on success, negative error code on failure
	 */
	int getname(void *addr, size_t *addrlen) const;

	// Communication Operations - Message Passing

	/**
	 * @brief Send message to remote endpoint
	 *
	 * Wrapper for fi_send() that provides type safety and consistent interface.
	 *
	 * @param buf Buffer containing message data
	 * @param len Length of message
	 * @param desc Memory descriptor for local buffer
	 * @param dest_addr Destination fabric address
	 * @param context Context for asynchronous operation
	 * @return 0 on success, -FI_EAGAIN if busy, negative error code on failure
	 */
	ssize_t send(const void *buf, size_t len, void *desc, fi_addr_t dest_addr, void *context) const;

	/**
	 * @brief Receive message from remote endpoint
	 *
	 * Wrapper for fi_recv() that provides type safety and consistent interface.
	 *
	 * @param buf Buffer to store received message
	 * @param len Length of receive buffer
	 * @param desc Memory descriptor for local buffer
	 * @param src_addr Source fabric address (or FI_ADDR_UNSPEC)
	 * @param context Context for asynchronous operation
	 * @return 0 on success, -FI_EAGAIN if busy, negative error code on failure
	 */
	ssize_t recv(void *buf, size_t len, void *desc, fi_addr_t src_addr, void *context) const;

	/**
	 * @brief Send tagged message to remote endpoint
	 *
	 * Wrapper for fi_tsend() that provides type safety and consistent interface.
	 *
	 * @param buf Buffer containing message data
	 * @param len Length of message
	 * @param desc Memory descriptor for local buffer
	 * @param dest_addr Destination fabric address
	 * @param tag Message tag for matching
	 * @param context Context for asynchronous operation
	 * @return 0 on success, -FI_EAGAIN if busy, negative error code on failure
	 */
	ssize_t tsend(const void *buf, size_t len, void *desc, fi_addr_t dest_addr, uint64_t tag, void *context) const;

	/**
	 * @brief Receive tagged message from remote endpoint
	 *
	 * Wrapper for fi_trecv() that provides type safety and consistent interface.
	 *
	 * @param buf Buffer to store received message
	 * @param len Length of receive buffer
	 * @param desc Memory descriptor for local buffer
	 * @param src_addr Source fabric address (or FI_ADDR_UNSPEC)
	 * @param tag Message tag for matching
	 * @param ignore Tag ignore mask
	 * @param context Context for asynchronous operation
	 * @return 0 on success, -FI_EAGAIN if busy, negative error code on failure
	 */
	ssize_t trecv(void *buf, size_t len, void *desc, fi_addr_t src_addr, uint64_t tag, uint64_t ignore, void *context) const;

	/**
	 * @brief Send message with immediate data to remote endpoint
	 *
	 * Wrapper for fi_senddata() that provides type safety and consistent interface.
	 *
	 * @param buf Buffer containing message data
	 * @param len Length of message
	 * @param desc Memory descriptor for local buffer
	 * @param data Immediate data to send
	 * @param dest_addr Destination fabric address
	 * @param context Context for asynchronous operation
	 * @return 0 on success, -FI_EAGAIN if busy, negative error code on failure
	 */
	ssize_t senddata(const void *buf, size_t len, void *desc, uint64_t data, fi_addr_t dest_addr, void *context) const;

	/**
	 * @brief Receive message using message structure
	 *
	 * Wrapper for fi_recvmsg() that provides type safety and consistent interface.
	 *
	 * @param msg Message structure containing receive parameters
	 * @param flags Operation flags
	 * @return 0 on success, -FI_EAGAIN if busy, negative error code on failure
	 */
	ssize_t recvmsg(const struct fi_msg *msg, uint64_t flags) const;

	// Communication Operations - RMA (Remote Memory Access)

	/**
	 * @brief Read data from remote memory
	 *
	 * Wrapper for fi_read() that provides type safety and consistent interface.
	 *
	 * @param buf Local buffer to store read data
	 * @param len Length of data to read
	 * @param desc Memory descriptor for local buffer
	 * @param src_addr Source fabric address
	 * @param addr Remote memory address
	 * @param key Remote memory key
	 * @param context Context for asynchronous operation
	 * @return 0 on success, -FI_EAGAIN if busy, negative error code on failure
	 */
	ssize_t read(void *buf, size_t len, void *desc, fi_addr_t src_addr, uint64_t addr, uint64_t key, void *context) const;

	/**
	 * @brief Write data to remote memory
	 *
	 * Wrapper for fi_write() that provides type safety and consistent interface.
	 *
	 * @param buf Local buffer containing data to write
	 * @param len Length of data to write
	 * @param desc Memory descriptor for local buffer
	 * @param dest_addr Destination fabric address
	 * @param addr Remote memory address
	 * @param key Remote memory key
	 * @param context Context for asynchronous operation
	 * @return 0 on success, -FI_EAGAIN if busy, negative error code on failure
	 */
	ssize_t write(const void *buf, size_t len, void *desc, fi_addr_t dest_addr, uint64_t addr, uint64_t key, void *context) const;

	/**
	 * @brief Write data to remote memory with immediate data
	 *
	 * Wrapper for fi_writedata() that provides type safety and consistent interface.
	 *
	 * @param buf Local buffer containing data to write
	 * @param len Length of data to write
	 * @param desc Memory descriptor for local buffer
	 * @param data Immediate data to send
	 * @param dest_addr Destination fabric address
	 * @param addr Remote memory address
	 * @param key Remote memory key
	 * @param context Context for asynchronous operation
	 * @return 0 on success, -FI_EAGAIN if busy, negative error code on failure
	 */
	ssize_t writedata(const void *buf, size_t len, void *desc, uint64_t data, fi_addr_t dest_addr, uint64_t addr, uint64_t key, void *context) const;

	/**
	 * @brief Write data using RMA message structure
	 *
	 * Wrapper for fi_writemsg() that provides type safety and consistent interface.
	 *
	 * @param msg RMA message structure containing write parameters
	 * @param flags Operation flags
	 * @return 0 on success, -FI_EAGAIN if busy, negative error code on failure
	 */
	ssize_t writemsg(const struct fi_msg_rma *msg, uint64_t flags) const;

	/**
	 * @brief Get direct access to the underlying fid_ep
	 *
	 * Provides access to the raw Libfabric endpoint handle for compatibility
	 * with existing code that expects fid_ep*.
	 *
	 * @return Pointer to the underlying fid_ep, or nullptr if invalid
	 */
	struct fid_ep *get() const noexcept;

	/**
	 * @brief Arrow operator for direct access to fid_ep members
	 *
	 * Allows direct access to fid_ep members using -> syntax.
	 *
	 * @return Pointer to the underlying fid_ep
	 */
	struct fid_ep *operator->() const noexcept;

	/**
	 * @brief Check if the endpoint is valid
	 *
	 * @return true if the endpoint is valid and can be used, false otherwise
	 */
	bool is_valid() const noexcept;

};

/**
 * @brief RAII wrapper for Libfabric Memory Region (fid_mr)
 *
 * This class provides automatic resource management for Libfabric memory regions,
 * ensuring proper cleanup and exception safety. The wrapper is designed to be used
 * with shared_ptr for lifetime management and supports move semantics.
 *
 * Key design principles:
 * - RAII compliance: Constructor acquires resource, destructor releases it
 * - Move-only semantics: Prevents accidental copying and double-close
 * - Dependency injection: Takes domain wrapper as shared_ptr parameter
 * - Exception safety: Constructor throws on failure, destructor is noexcept
 * - Resource isolation: Only manages MR, not other Libfabric resources
 */
class ofi_mr_raii {
	private:
	struct fid_mr *mr_;

	public:
	/**
	 * @brief Constructor - registers a memory region
	 *
	 * @param domain Shared pointer to domain wrapper (ensures domain lifetime)
	 * @param attr MR attributes for registration
	 * @param flags Registration flags
	 * @throws std::runtime_error if fi_mr_regattr fails
	 */
	explicit ofi_mr_raii(std::shared_ptr<ofi_domain_raii> domain,
			     struct fi_mr_attr &attr, uint64_t flags);

	/**
	 * @brief Move constructor
	 *
	 * Transfers ownership of the MR resource from other to this instance.
	 * The other instance will be left in a valid but empty state.
	 *
	 * @param other Source object to move from
	 */
	ofi_mr_raii(ofi_mr_raii &&other) noexcept;

	/**
	 * @brief Move assignment operator
	 *
	 * Transfers ownership of the MR resource from other to this instance.
	 * If this instance currently owns an MR, it will be properly closed first.
	 *
	 * @param other Source object to move from
	 * @return Reference to this instance
	 */
	ofi_mr_raii &operator=(ofi_mr_raii &&other) noexcept;

	/**
	 * @brief Destructor - automatically deregisters the memory region
	 *
	 * Calls fi_close on the underlying fid_mr if it's valid.
	 * This destructor is noexcept to ensure proper RAII behavior.
	 */
	~ofi_mr_raii() noexcept;

	// Delete copy constructor and copy assignment to prevent double-close
	ofi_mr_raii(const ofi_mr_raii &) = delete;
	ofi_mr_raii &operator=(const ofi_mr_raii &) = delete;

	/**
	 * @brief Bind memory region to endpoint (for endpoint_mr providers)
	 *
	 * Wrapper for fi_mr_bind() that provides type safety and consistent interface.
	 * Only needed for providers that require endpoint binding.
	 *
	 * @param ep Shared pointer to endpoint wrapper to bind
	 * @param flags Binding flags (typically 0)
	 * @return 0 on success, negative error code on failure
	 */
	int bind_ep(std::shared_ptr<ofi_ep_raii> ep, uint64_t flags);

	/**
	 * @brief Bind memory region to resource
	 *
	 * Wrapper for fi_mr_bind() that provides type safety and consistent interface.
	 *
	 * @param fid Resource to bind to memory region
	 * @param flags Binding flags
	 * @return 0 on success, negative error code on failure
	 */
	int bind(struct fid *fid, uint64_t flags) const;

	/**
	 * @brief Enable memory region after binding (for endpoint_mr providers)
	 *
	 * Wrapper for fi_mr_enable() that provides type safety and consistent interface.
	 * Only needed for providers that require endpoint binding.
	 *
	 * @return 0 on success, negative error code on failure
	 */
	int enable() const;

	/**
	 * @brief Get memory region key for remote access
	 *
	 * Wrapper for fi_mr_key() that provides type safety and consistent interface.
	 *
	 * @return MR key for remote access operations
	 */
	uint64_t key() const noexcept;

	/**
	 * @brief Get memory region descriptor for local operations
	 *
	 * Wrapper for fi_mr_desc() that provides type safety and consistent interface.
	 *
	 * @return MR descriptor for local operations
	 */
	void *desc() const noexcept;

	/**
	 * @brief Get direct access to the underlying fid_mr
	 *
	 * Provides access to the raw Libfabric MR handle for compatibility
	 * with existing code that expects fid_mr*.
	 *
	 * @return Pointer to the underlying fid_mr, or nullptr if invalid
	 */
	struct fid_mr *get() const noexcept;

	/**
	 * @brief Arrow operator for direct access to fid_mr members
	 *
	 * Allows direct access to fid_mr members using -> syntax.
	 *
	 * @return Pointer to the underlying fid_mr
	 */
	struct fid_mr *operator->() const noexcept;

	/**
	 * @brief Check if the MR is valid
	 *
	 * @return true if the MR is valid and can be used, false otherwise
	 */
	bool is_valid() const noexcept;

};

/**
 * @brief Convenience type alias for shared fabric wrapper
 *
 * This type alias makes it easier to work with shared fabric wrappers
 * throughout the codebase.
 */
using shared_fabric_raii = std::shared_ptr<ofi_fabric_raii>;

/**
 * @brief Factory function to create a shared fabric wrapper
 *
 * Convenience function that creates a shared_ptr to ofi_fabric_raii.
 * This is the recommended way to create fabric wrappers.
 *
 * @param fabric_attr Libfabric fabric attributes for creation
 * @return Shared pointer to the created fabric wrapper
 * @throws std::runtime_error if fabric creation fails
 */
inline shared_fabric_raii make_shared_fabric(struct fi_fabric_attr *fabric_attr)
{
	return std::make_shared<ofi_fabric_raii>(fabric_attr);
}

/**
 * @brief Convenience type alias for shared domain wrapper
 *
 * This type alias makes it easier to work with shared domain wrappers
 * throughout the codebase.
 */
using shared_domain_raii = std::shared_ptr<ofi_domain_raii>;

/**
 * @brief Factory function to create a shared domain wrapper
 *
 * Convenience function that creates a shared_ptr to ofi_domain_raii.
 * This is the recommended way to create domain wrappers.
 *
 * @param fabric Shared pointer to fabric wrapper
 * @param info Libfabric info structure for domain creation
 * @return Shared pointer to the created domain wrapper
 * @throws std::runtime_error if domain creation fails
 */
inline shared_domain_raii make_shared_domain(std::shared_ptr<ofi_fabric_raii> fabric,
					     struct fi_info *info)
{
	return std::make_shared<ofi_domain_raii>(fabric, info);
}

/**
 * @brief Convenience type alias for shared CQ wrapper
 *
 * This type alias makes it easier to work with shared CQ wrappers
 * throughout the codebase.
 */
using shared_cq_raii = std::shared_ptr<ofi_cq_raii>;

/**
 * @brief Factory function to create a shared CQ wrapper
 *
 * Convenience function that creates a shared_ptr to ofi_cq_raii.
 * This is the recommended way to create CQ wrappers.
 *
 * @param domain Shared pointer to domain wrapper
 * @param attr CQ attributes for creation
 * @return Shared pointer to the created CQ wrapper
 * @throws std::runtime_error if CQ creation fails
 */
inline shared_cq_raii make_shared_cq(std::shared_ptr<ofi_domain_raii> domain,
				     struct fi_cq_attr &attr)
{
	return std::make_shared<ofi_cq_raii>(domain, attr);
}

/**
 * @brief Convenience type alias for shared AV wrapper
 *
 * This type alias makes it easier to work with shared AV wrappers
 * throughout the codebase.
 */
using shared_av_raii = std::shared_ptr<ofi_av_raii>;

/**
 * @brief Factory function to create a shared AV wrapper
 *
 * Convenience function that creates a shared_ptr to ofi_av_raii.
 * This is the recommended way to create AV wrappers.
 *
 * @param domain Shared pointer to domain wrapper
 * @param attr AV attributes for creation
 * @return Shared pointer to the created AV wrapper
 * @throws std::runtime_error if AV creation fails
 */
inline shared_av_raii make_shared_av(std::shared_ptr<ofi_domain_raii> domain,
				     struct fi_av_attr &attr)
{
	return std::make_shared<ofi_av_raii>(domain, attr);
}

/**
 * @brief Convenience type alias for shared endpoint wrapper
 *
 * This type alias makes it easier to work with shared endpoint wrappers
 * throughout the codebase.
 */
using shared_ep_raii = std::shared_ptr<ofi_ep_raii>;

/**
 * @brief Factory function to create a shared endpoint wrapper
 *
 * Convenience function that creates a shared_ptr to ofi_ep_raii.
 * This is the recommended way to create endpoint wrappers.
 *
 * @param domain Shared pointer to domain wrapper
 * @param info Libfabric info structure for endpoint creation
 * @return Shared pointer to the created endpoint wrapper
 * @throws std::runtime_error if endpoint creation fails
 */
inline shared_ep_raii make_shared_ep(std::shared_ptr<ofi_domain_raii> domain,
				     struct fi_info *info)
{
	return std::make_shared<ofi_ep_raii>(domain, info);
}

/**
 * @brief Convenience type alias for shared MR wrapper
 *
 * This type alias makes it easier to work with shared MR wrappers
 * throughout the codebase.
 */
using shared_mr_raii = std::shared_ptr<ofi_mr_raii>;

/**
 * @brief Factory function to create a shared MR wrapper
 *
 * Convenience function that creates a shared_ptr to ofi_mr_raii.
 * This is the recommended way to create MR wrappers.
 *
 * @param domain Shared pointer to domain wrapper
 * @param attr MR attributes for registration
 * @param flags Registration flags
 * @return Shared pointer to the created MR wrapper
 * @throws std::runtime_error if MR registration fails
 */
inline shared_mr_raii make_shared_mr(std::shared_ptr<ofi_domain_raii> domain,
				     struct fi_mr_attr &attr, uint64_t flags)
{
	return std::make_shared<ofi_mr_raii>(domain, attr, flags);
}

#endif // NCCL_OFI_OFIRAII_H_
