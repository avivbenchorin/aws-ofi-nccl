/*
 * Copyright (c) 2023-2024 Amazon.com, Inc. or its affiliates. All rights reserved.
 */

#include "config.h"

#include <stdexcept>
#include <sstream>

#include <rdma/fabric.h>

#include "ofi/nccl_ofi_ofiraii.h"
#include "nccl_ofi_log.h"

// Constructor - creates and opens a fabric
ofi_fabric_raii::ofi_fabric_raii(struct fi_fabric_attr* fabric_attr)
    : fabric_(nullptr)
{
    if (!fabric_attr) {
        throw std::runtime_error("ofi_fabric_raii: fabric_attr is null");
    }

    int ret = fi_fabric(fabric_attr, &fabric_, nullptr);
    if (ret != 0) {
        std::ostringstream oss;
        oss << "ofi_fabric_raii: fi_fabric failed with RC: " << ret 
            << ", Error: " << fi_strerror(-ret);
        NCCL_OFI_WARN("%s", oss.str().c_str());
        throw std::runtime_error(oss.str());
    }

    NCCL_OFI_TRACE(NCCL_NET, "Created fabric wrapper with fid_fabric: %p", fabric_);
}

// Move constructor
ofi_fabric_raii::ofi_fabric_raii(ofi_fabric_raii&& other) noexcept
    : fabric_(other.fabric_)
{
    other.fabric_ = nullptr;
    NCCL_OFI_TRACE(NCCL_NET, "Moved fabric wrapper, fid_fabric: %p", fabric_);
}

// Move assignment operator
ofi_fabric_raii& ofi_fabric_raii::operator=(ofi_fabric_raii&& other) noexcept
{
    if (this != &other) {
        // Clean up current resource if we have one
        if (fabric_) {
            NCCL_OFI_TRACE(NCCL_NET, "Closing fabric in move assignment, fid_fabric: %p", fabric_);
            int ret = fi_close(&fabric_->fid);
            if (ret != 0) {
                NCCL_OFI_WARN("Failed to close fabric in move assignment. RC: %d, Error: %s",
                              ret, fi_strerror(-ret));
            }
        }

        // Transfer ownership
        fabric_ = other.fabric_;
        other.fabric_ = nullptr;
        
        NCCL_OFI_TRACE(NCCL_NET, "Move assigned fabric wrapper, fid_fabric: %p", fabric_);
    }
    return *this;
}

// Destructor - automatically closes the fabric
ofi_fabric_raii::~ofi_fabric_raii() noexcept
{
    if (fabric_) {
        NCCL_OFI_TRACE(NCCL_NET, "Destroying fabric wrapper, closing fid_fabric: %p", fabric_);
        int ret = fi_close(&fabric_->fid);
        if (ret != 0) {
            NCCL_OFI_WARN("Failed to close fabric in destructor. RC: %d, Error: %s",
                          ret, fi_strerror(-ret));
        }
    }
}

// Get direct access to the underlying fid_fabric
struct fid_fabric* ofi_fabric_raii::get() const noexcept
{
    return fabric_;
}

// Arrow operator for direct access to fid_fabric members
struct fid_fabric* ofi_fabric_raii::operator->() const noexcept
{
    return fabric_;
}

// Dereference operator for direct access to fid_fabric
struct fid_fabric& ofi_fabric_raii::operator*() const noexcept
{
    return *fabric_;
}

// Check if the fabric is valid
bool ofi_fabric_raii::is_valid() const noexcept
{
    return fabric_ != nullptr;
}

// Constructor - creates and opens a domain
ofi_domain_raii::ofi_domain_raii(std::shared_ptr<ofi_fabric_raii> fabric,
				 struct fi_info *info)
	: domain_(nullptr)
{
	if (!fabric) {
		throw std::runtime_error("ofi_domain_raii: fabric wrapper is null");
	}

	if (!info) {
		throw std::runtime_error("ofi_domain_raii: fi_info is null");
	}

	struct fid_fabric *ofi_fabric = fabric->get();
	if (!ofi_fabric) {
		throw std::runtime_error("ofi_domain_raii: underlying fabric is null");
	}

	int ret = fi_domain(ofi_fabric, info, &domain_, nullptr);
	if (ret != 0) {
		std::ostringstream oss;
		oss << "ofi_domain_raii: fi_domain failed with RC: " << ret
			<< ", Error: " << fi_strerror(-ret);
		NCCL_OFI_WARN("%s", oss.str().c_str());
		throw std::runtime_error(oss.str());
	}

	NCCL_OFI_TRACE(NCCL_NET, "Created domain wrapper with fid_domain: %p", domain_);
}

// Move constructor
ofi_domain_raii::ofi_domain_raii(ofi_domain_raii &&other) noexcept
	: domain_(other.domain_)
{
	other.domain_ = nullptr;
	NCCL_OFI_TRACE(NCCL_NET, "Moved domain wrapper, fid_domain: %p", domain_);
}

// Move assignment operator
ofi_domain_raii &ofi_domain_raii::operator=(ofi_domain_raii &&other) noexcept
{
	if (this != &other) {
		// Clean up current resource if we have one
		if (domain_) {
			NCCL_OFI_TRACE(NCCL_NET, "Closing domain in move assignment, fid_domain: %p", domain_);
			int ret = fi_close(&domain_->fid);
			if (ret != 0) {
				NCCL_OFI_WARN("Failed to close domain in move assignment. RC: %d, Error: %s",
					      ret, fi_strerror(-ret));
			}
		}

		// Transfer ownership
		domain_ = other.domain_;
		other.domain_ = nullptr;

		NCCL_OFI_TRACE(NCCL_NET, "Move assigned domain wrapper, fid_domain: %p", domain_);
	}
	return *this;
}

// Destructor - automatically closes the domain
ofi_domain_raii::~ofi_domain_raii() noexcept
{
	if (domain_) {
		NCCL_OFI_TRACE(NCCL_NET, "Destroying domain wrapper, closing fid_domain: %p", domain_);
		int ret = fi_close(&domain_->fid);
		if (ret != 0) {
			NCCL_OFI_WARN("Failed to close domain in destructor. RC: %d, Error: %s",
				      ret, fi_strerror(-ret));
		}
	}
}

// Get direct access to the underlying fid_domain
struct fid_domain *ofi_domain_raii::get() const noexcept
{
	return domain_;
}

// Arrow operator for direct access to fid_domain members
struct fid_domain *ofi_domain_raii::operator->() const noexcept
{
	return domain_;
}

// Dereference operator for direct access to fid_domain
struct fid_domain &ofi_domain_raii::operator*() const noexcept
{
	return *domain_;
}

// Check if the domain is valid
bool ofi_domain_raii::is_valid() const noexcept
{
	return domain_ != nullptr;
}

// Constructor - creates and opens a completion queue
ofi_cq_raii::ofi_cq_raii(std::shared_ptr<ofi_domain_raii> domain,
			 struct fi_cq_attr &attr)
	: cq_(nullptr)
{
	if (!domain) {
		throw std::runtime_error("ofi_cq_raii: domain wrapper is null");
	}

	struct fid_domain *ofi_domain = domain->get();
	if (!ofi_domain) {
		throw std::runtime_error("ofi_cq_raii: underlying domain is null");
	}

	int ret = fi_cq_open(ofi_domain, &attr, &cq_, nullptr);
	if (ret != 0) {
		std::ostringstream oss;
		oss << "ofi_cq_raii: fi_cq_open failed with RC: " << ret
			<< ", Error: " << fi_strerror(-ret);
		NCCL_OFI_WARN("%s", oss.str().c_str());
		throw std::runtime_error(oss.str());
	}

	NCCL_OFI_TRACE(NCCL_NET, "Created CQ wrapper with fid_cq: %p", cq_);
}

// Move constructor
ofi_cq_raii::ofi_cq_raii(ofi_cq_raii &&other) noexcept
	: cq_(other.cq_)
{
	other.cq_ = nullptr;
	NCCL_OFI_TRACE(NCCL_NET, "Moved CQ wrapper, fid_cq: %p", cq_);
}

// Move assignment operator
ofi_cq_raii &ofi_cq_raii::operator=(ofi_cq_raii &&other) noexcept
{
	if (this != &other) {
		// Clean up current resource if we have one
		if (cq_) {
			NCCL_OFI_TRACE(NCCL_NET, "Closing CQ in move assignment, fid_cq: %p", cq_);
			int ret = fi_close(&cq_->fid);
			if (ret != 0) {
				NCCL_OFI_WARN("Failed to close CQ in move assignment. RC: %d, Error: %s",
					      ret, fi_strerror(-ret));
			}
		}

		// Transfer ownership
		cq_ = other.cq_;
		other.cq_ = nullptr;

		NCCL_OFI_TRACE(NCCL_NET, "Move assigned CQ wrapper, fid_cq: %p", cq_);
	}
	return *this;
}

// Destructor - automatically closes the completion queue
ofi_cq_raii::~ofi_cq_raii() noexcept
{
	if (cq_) {
		NCCL_OFI_TRACE(NCCL_NET, "Destroying CQ wrapper, closing fid_cq: %p", cq_);
		int ret = fi_close(&cq_->fid);
		if (ret != 0) {
			NCCL_OFI_WARN("Failed to close CQ in destructor. RC: %d, Error: %s",
				      ret, fi_strerror(-ret));
		}
	}
}

// Read completion entries from the queue
ssize_t ofi_cq_raii::read(void *buf, size_t count) const
{
	if (!cq_) {
		NCCL_OFI_WARN("Attempted to read from invalid CQ");
		return -FI_EINVAL;
	}

	ssize_t ret = fi_cq_read(cq_, buf, count);

	// Only log on actual errors, not on -FI_EAGAIN (no completions available)
	if (ret < 0 && ret != -FI_EAGAIN && ret != -FI_EAVAIL) {
		NCCL_OFI_TRACE(NCCL_NET, "fi_cq_read returned error: %zd (%s)",
			       ret, fi_strerror(-ret));
	}

	return ret;
}

// Read error entries from the queue
ssize_t ofi_cq_raii::read_error(struct fi_cq_err_entry *buf, uint64_t flags) const
{
	if (!cq_) {
		NCCL_OFI_WARN("Attempted to read error from invalid CQ");
		return -FI_EINVAL;
	}

	ssize_t ret = fi_cq_readerr(cq_, buf, flags);

	if (ret < 0 && ret != -FI_EAGAIN) {
		NCCL_OFI_TRACE(NCCL_NET, "fi_cq_readerr returned error: %zd (%s)",
			       ret, fi_strerror(-ret));
	}

	return ret;
}

// Convert provider error to human-readable string
const char *ofi_cq_raii::strerror(int prov_errno, const void *err_data,
				  char *buf, size_t len) const
{
	if (!cq_) {
		NCCL_OFI_WARN("Attempted to get error string from invalid CQ");
		return "Invalid CQ";
	}

	return fi_cq_strerror(cq_, prov_errno, err_data, buf, len);
}

// Get direct access to the underlying fid_cq
struct fid_cq *ofi_cq_raii::get() const noexcept
{
	return cq_;
}

// Arrow operator for direct access to fid_cq members
struct fid_cq *ofi_cq_raii::operator->() const noexcept
{
	return cq_;
}

// Check if the CQ is valid
bool ofi_cq_raii::is_valid() const noexcept
{
	return cq_ != nullptr;
}

// Constructor - creates and opens an address vector
ofi_av_raii::ofi_av_raii(std::shared_ptr<ofi_domain_raii> domain,
                         struct fi_av_attr &attr)
    : av_(nullptr)
{
    if (!domain) {
        throw std::runtime_error("ofi_av_raii: domain wrapper is null");
    }

    struct fid_domain *ofi_domain = domain->get();
    if (!ofi_domain) {
        throw std::runtime_error("ofi_av_raii: underlying domain is null");
    }

    int ret = fi_av_open(ofi_domain, &attr, &av_, nullptr);
    if (ret != 0) {
        std::ostringstream oss;
        oss << "ofi_av_raii: fi_av_open failed with RC: " << ret
            << ", Error: " << fi_strerror(-ret);
        NCCL_OFI_WARN("%s", oss.str().c_str());
        throw std::runtime_error(oss.str());
    }

    NCCL_OFI_TRACE(NCCL_NET, "Created AV wrapper with fid_av: %p", av_);
}

// Move constructor
ofi_av_raii::ofi_av_raii(ofi_av_raii &&other) noexcept
    : av_(other.av_)
{
    other.av_ = nullptr;
    NCCL_OFI_TRACE(NCCL_NET, "Moved AV wrapper, fid_av: %p", av_);
}

// Move assignment operator
ofi_av_raii &ofi_av_raii::operator=(ofi_av_raii &&other) noexcept
{
    if (this != &other) {
        // Clean up current resource if we have one
        if (av_) {
            NCCL_OFI_TRACE(NCCL_NET, "Closing AV in move assignment, fid_av: %p", av_);
            int ret = fi_close(&av_->fid);
            if (ret != 0) {
                NCCL_OFI_WARN("Failed to close AV in move assignment. RC: %d, Error: %s",
                              ret, fi_strerror(-ret));
            }
        }

        // Transfer ownership
        av_ = other.av_;
        other.av_ = nullptr;

        NCCL_OFI_TRACE(NCCL_NET, "Move assigned AV wrapper, fid_av: %p", av_);
    }
    return *this;
}

// Destructor - automatically closes the address vector
ofi_av_raii::~ofi_av_raii() noexcept
{
    if (av_) {
        NCCL_OFI_TRACE(NCCL_NET, "Destroying AV wrapper, closing fid_av: %p", av_);
        int ret = fi_close(&av_->fid);
        if (ret != 0) {
            NCCL_OFI_WARN("Failed to close AV in destructor. RC: %d, Error: %s",
                          ret, fi_strerror(-ret));
        }
    }
}

// Insert addresses into the address vector
int ofi_av_raii::insert(const void *addr, size_t count, fi_addr_t *fi_addr,
                        uint64_t flags, void *context) const
{
    if (!av_) {
        NCCL_OFI_WARN("Attempted to insert into invalid AV");
        return -FI_EINVAL;
    }

    int ret = fi_av_insert(av_, addr, count, fi_addr, flags, context);
    if (ret < 0) {
        NCCL_OFI_TRACE(NCCL_NET, "fi_av_insert returned error: %d (%s)",
                       ret, fi_strerror(-ret));
    }

    return ret;
}

// Get direct access to the underlying fid_av
struct fid_av *ofi_av_raii::get() const noexcept
{
    return av_;
}

// Arrow operator for direct access to fid_av members
struct fid_av *ofi_av_raii::operator->() const noexcept
{
    return av_;
}

// Check if the AV is valid
bool ofi_av_raii::is_valid() const noexcept
{
    return av_ != nullptr;
}

// Constructor - creates and opens an endpoint
ofi_ep_raii::ofi_ep_raii(std::shared_ptr<ofi_domain_raii> domain,
                         struct fi_info *info)
    : ep_(nullptr)
{
    if (!domain) {
        throw std::runtime_error("ofi_ep_raii: domain wrapper is null");
    }

    if (!info) {
        throw std::runtime_error("ofi_ep_raii: fi_info is null");
    }

    struct fid_domain *ofi_domain = domain->get();
    if (!ofi_domain) {
        throw std::runtime_error("ofi_ep_raii: underlying domain is null");
    }

    int ret = fi_endpoint(ofi_domain, info, &ep_, nullptr);
    if (ret != 0) {
        std::ostringstream oss;
        oss << "ofi_ep_raii: fi_endpoint failed with RC: " << ret
            << ", Error: " << fi_strerror(-ret);
        NCCL_OFI_WARN("%s", oss.str().c_str());
        throw std::runtime_error(oss.str());
    }

    NCCL_OFI_TRACE(NCCL_NET, "Created EP wrapper with fid_ep: %p", ep_);
}

// Move constructor
ofi_ep_raii::ofi_ep_raii(ofi_ep_raii &&other) noexcept
    : ep_(other.ep_)
{
    other.ep_ = nullptr;
    NCCL_OFI_TRACE(NCCL_NET, "Moved EP wrapper, fid_ep: %p", ep_);
}

// Move assignment operator
ofi_ep_raii &ofi_ep_raii::operator=(ofi_ep_raii &&other) noexcept
{
    if (this != &other) {
        // Clean up current resource if we have one
        if (ep_) {
            NCCL_OFI_TRACE(NCCL_NET, "Closing EP in move assignment, fid_ep: %p", ep_);
            int ret = fi_close(&ep_->fid);
            if (ret != 0) {
                NCCL_OFI_WARN("Failed to close EP in move assignment. RC: %d, Error: %s",
                              ret, fi_strerror(-ret));
            }
        }

        // Transfer ownership
        ep_ = other.ep_;
        other.ep_ = nullptr;

        NCCL_OFI_TRACE(NCCL_NET, "Move assigned EP wrapper, fid_ep: %p", ep_);
    }
    return *this;
}

// Destructor - automatically closes the endpoint
ofi_ep_raii::~ofi_ep_raii() noexcept
{
    if (ep_) {
        NCCL_OFI_TRACE(NCCL_NET, "Destroying EP wrapper, closing fid_ep: %p", ep_);
        int ret = fi_close(&ep_->fid);
        if (ret != 0) {
            NCCL_OFI_WARN("Failed to close EP in destructor. RC: %d, Error: %s",
                          ret, fi_strerror(-ret));
        }
    }
}

// Bind completion queue to endpoint
int ofi_ep_raii::bind_cq(std::shared_ptr<ofi_cq_raii> cq, uint64_t flags)
{
    if (!cq) {
        NCCL_OFI_WARN("Attempted to bind null CQ to EP");
        return -FI_EINVAL;
    }

    return bind(&cq->get()->fid, flags);
}

// Bind address vector to endpoint
int ofi_ep_raii::bind_av(std::shared_ptr<ofi_av_raii> av, uint64_t flags)
{
    if (!av) {
        NCCL_OFI_WARN("Attempted to bind null AV to EP");
        return -FI_EINVAL;
    }

    return bind(&av->get()->fid, flags);
}

// Bind endpoint to a resource (CQ, AV, etc.)
int ofi_ep_raii::bind(struct fid *fid, uint64_t flags) const
{
    if (!ep_) {
        NCCL_OFI_WARN("Attempted to bind invalid EP");
        return -FI_EINVAL;
    }

    int ret = fi_ep_bind(ep_, fid, flags);
    if (ret != 0) {
        NCCL_OFI_TRACE(NCCL_NET, "fi_ep_bind returned error: %d (%s)",
                       ret, fi_strerror(-ret));
    }

    return ret;
}

// Enable the endpoint
int ofi_ep_raii::enable() const
{
    if (!ep_) {
        NCCL_OFI_WARN("Attempted to enable invalid EP");
        return -FI_EINVAL;
    }

    int ret = fi_enable(ep_);
    if (ret != 0) {
        NCCL_OFI_TRACE(NCCL_NET, "fi_enable returned error: %d (%s)",
                       ret, fi_strerror(-ret));
    }

    return ret;
}

// Cancel outstanding operations
int ofi_ep_raii::cancel(void *context) const
{
    if (!ep_) {
        NCCL_OFI_WARN("Attempted to cancel on invalid EP");
        return -FI_EINVAL;
    }

    int ret = fi_cancel(&ep_->fid, context);
    if (ret != 0) {
        NCCL_OFI_TRACE(NCCL_NET, "fi_cancel returned error: %d (%s)",
                       ret, fi_strerror(-ret));
    }

    return ret;
}

// Set endpoint option
int ofi_ep_raii::setopt(int level, int optname, const void *optval, size_t optlen) const
{
    if (!ep_) {
        NCCL_OFI_WARN("Attempted to set option on invalid EP");
        return -FI_EINVAL;
    }

    int ret = fi_setopt(&ep_->fid, level, optname, optval, optlen);
    if (ret != 0) {
        NCCL_OFI_TRACE(NCCL_NET, "fi_setopt returned error: %d (%s)",
                       ret, fi_strerror(-ret));
    }

    return ret;
}

// Get endpoint option
int ofi_ep_raii::getopt(int level, int optname, void *optval, size_t *optlen) const
{
    if (!ep_) {
        NCCL_OFI_WARN("Attempted to get option from invalid EP");
        return -FI_EINVAL;
    }

    if (!optval || !optlen) {
        NCCL_OFI_WARN("Invalid parameters: optval or optlen is null");
        return -FI_EINVAL;
    }

    int ret = fi_getopt(&ep_->fid, level, optname, optval, optlen);
    if (ret != 0) {
        NCCL_OFI_TRACE(NCCL_NET, "fi_getopt returned error: %d (%s)",
                       ret, fi_strerror(-ret));
    }

    return ret;
}

// Get endpoint name/address
int ofi_ep_raii::getname(void *addr, size_t *addrlen) const
{
    if (!ep_) {
        NCCL_OFI_WARN("Attempted to get name from invalid EP");
        return -FI_EINVAL;
    }

    if (!addr || !addrlen) {
        NCCL_OFI_WARN("Invalid parameters: addr or addrlen is null");
        return -FI_EINVAL;
    }

    int ret = fi_getname(&ep_->fid, addr, addrlen);
    if (ret != 0) {
        NCCL_OFI_TRACE(NCCL_NET, "fi_getname returned error: %d (%s)",
                       ret, fi_strerror(-ret));
    }

    return ret;
}

// Get direct access to the underlying fid_ep
struct fid_ep *ofi_ep_raii::get() const noexcept
{
    return ep_;
}

// Arrow operator for direct access to fid_ep members
struct fid_ep *ofi_ep_raii::operator->() const noexcept
{
    return ep_;
}

// Communication Operations - Message Passing

// Send message to remote endpoint
ssize_t ofi_ep_raii::send(const void *buf, size_t len, void *desc, fi_addr_t dest_addr, void *context) const
{
    if (!ep_) {
        NCCL_OFI_WARN("Attempted send on invalid EP");
        return -FI_EINVAL;
    }

    ssize_t ret = fi_send(ep_, buf, len, desc, dest_addr, context);
    if (ret < 0 && ret != -FI_EAGAIN) {
        NCCL_OFI_TRACE(NCCL_NET, "fi_send returned error: %zd (%s)",
                       ret, fi_strerror(-ret));
    }

    return ret;
}

// Receive message from remote endpoint
ssize_t ofi_ep_raii::recv(void *buf, size_t len, void *desc, fi_addr_t src_addr, void *context) const
{
    if (!ep_) {
        NCCL_OFI_WARN("Attempted recv on invalid EP");
        return -FI_EINVAL;
    }

    ssize_t ret = fi_recv(ep_, buf, len, desc, src_addr, context);
    if (ret < 0 && ret != -FI_EAGAIN) {
        NCCL_OFI_TRACE(NCCL_NET, "fi_recv returned error: %zd (%s)",
                       ret, fi_strerror(-ret));
    }

    return ret;
}

// Send tagged message to remote endpoint
ssize_t ofi_ep_raii::tsend(const void *buf, size_t len, void *desc, fi_addr_t dest_addr, uint64_t tag, void *context) const
{
    if (!ep_) {
        NCCL_OFI_WARN("Attempted tsend on invalid EP");
        return -FI_EINVAL;
    }

    ssize_t ret = fi_tsend(ep_, buf, len, desc, dest_addr, tag, context);
    if (ret < 0 && ret != -FI_EAGAIN) {
        NCCL_OFI_TRACE(NCCL_NET, "fi_tsend returned error: %zd (%s)",
                       ret, fi_strerror(-ret));
    }

    return ret;
}

// Receive tagged message from remote endpoint
ssize_t ofi_ep_raii::trecv(void *buf, size_t len, void *desc, fi_addr_t src_addr, uint64_t tag, uint64_t ignore, void *context) const
{
    if (!ep_) {
        NCCL_OFI_WARN("Attempted trecv on invalid EP");
        return -FI_EINVAL;
    }

    ssize_t ret = fi_trecv(ep_, buf, len, desc, src_addr, tag, ignore, context);
    if (ret < 0 && ret != -FI_EAGAIN) {
        NCCL_OFI_TRACE(NCCL_NET, "fi_trecv returned error: %zd (%s)",
                       ret, fi_strerror(-ret));
    }

    return ret;
}

// Send message with immediate data to remote endpoint
ssize_t ofi_ep_raii::senddata(const void *buf, size_t len, void *desc, uint64_t data, fi_addr_t dest_addr, void *context) const
{
    if (!ep_) {
        NCCL_OFI_WARN("Attempted senddata on invalid EP");
        return -FI_EINVAL;
    }

    ssize_t ret = fi_senddata(ep_, buf, len, desc, data, dest_addr, context);
    if (ret < 0 && ret != -FI_EAGAIN) {
        NCCL_OFI_TRACE(NCCL_NET, "fi_senddata returned error: %zd (%s)",
                       ret, fi_strerror(-ret));
    }

    return ret;
}

// Receive message using message structure
ssize_t ofi_ep_raii::recvmsg(const struct fi_msg *msg, uint64_t flags) const
{
    if (!ep_) {
        NCCL_OFI_WARN("Attempted recvmsg on invalid EP");
        return -FI_EINVAL;
    }

    if (!msg) {
        NCCL_OFI_WARN("Invalid message structure provided to recvmsg");
        return -FI_EINVAL;
    }

    ssize_t ret = fi_recvmsg(ep_, msg, flags);
    if (ret < 0 && ret != -FI_EAGAIN) {
        NCCL_OFI_TRACE(NCCL_NET, "fi_recvmsg returned error: %zd (%s)",
                       ret, fi_strerror(-ret));
    }

    return ret;
}

// Communication Operations - RMA (Remote Memory Access)

// Read data from remote memory
ssize_t ofi_ep_raii::read(void *buf, size_t len, void *desc, fi_addr_t src_addr, uint64_t addr, uint64_t key, void *context) const
{
    if (!ep_) {
        NCCL_OFI_WARN("Attempted read on invalid EP");
        return -FI_EINVAL;
    }

    ssize_t ret = fi_read(ep_, buf, len, desc, src_addr, addr, key, context);
    if (ret < 0 && ret != -FI_EAGAIN) {
        NCCL_OFI_TRACE(NCCL_NET, "fi_read returned error: %zd (%s)",
                       ret, fi_strerror(-ret));
    }

    return ret;
}

// Write data to remote memory
ssize_t ofi_ep_raii::write(const void *buf, size_t len, void *desc, fi_addr_t dest_addr, uint64_t addr, uint64_t key, void *context) const
{
    if (!ep_) {
        NCCL_OFI_WARN("Attempted write on invalid EP");
        return -FI_EINVAL;
    }

    ssize_t ret = fi_write(ep_, buf, len, desc, dest_addr, addr, key, context);
    if (ret < 0 && ret != -FI_EAGAIN) {
        NCCL_OFI_TRACE(NCCL_NET, "fi_write returned error: %zd (%s)",
                       ret, fi_strerror(-ret));
    }

    return ret;
}

// Write data to remote memory with immediate data
ssize_t ofi_ep_raii::writedata(const void *buf, size_t len, void *desc, uint64_t data, fi_addr_t dest_addr, uint64_t addr, uint64_t key, void *context) const
{
    if (!ep_) {
        NCCL_OFI_WARN("Attempted writedata on invalid EP");
        return -FI_EINVAL;
    }

    ssize_t ret = fi_writedata(ep_, buf, len, desc, data, dest_addr, addr, key, context);
    if (ret < 0 && ret != -FI_EAGAIN) {
        NCCL_OFI_TRACE(NCCL_NET, "fi_writedata returned error: %zd (%s)",
                       ret, fi_strerror(-ret));
    }

    return ret;
}

// Write data using RMA message structure
ssize_t ofi_ep_raii::writemsg(const struct fi_msg_rma *msg, uint64_t flags) const
{
    if (!ep_) {
        NCCL_OFI_WARN("Attempted writemsg on invalid EP");
        return -FI_EINVAL;
    }

    if (!msg) {
        NCCL_OFI_WARN("Invalid RMA message structure provided to writemsg");
        return -FI_EINVAL;
    }

    ssize_t ret = fi_writemsg(ep_, msg, flags);
    if (ret < 0 && ret != -FI_EAGAIN) {
        NCCL_OFI_TRACE(NCCL_NET, "fi_writemsg returned error: %zd (%s)",
                       ret, fi_strerror(-ret));
    }

    return ret;
}

// Check if the EP is valid
bool ofi_ep_raii::is_valid() const noexcept
{
    return ep_ != nullptr;
}

// Constructor - registers memory region
ofi_mr_raii::ofi_mr_raii(std::shared_ptr<ofi_domain_raii> domain,
                         struct fi_mr_attr &attr, uint64_t flags)
    : mr_(nullptr)
{
    if (!domain) {
        throw std::runtime_error("ofi_mr_raii: domain wrapper is null");
    }

    struct fid_domain *ofi_domain = domain->get();
    if (!ofi_domain) {
        throw std::runtime_error("ofi_mr_raii: underlying domain is null");
    }

    int ret = fi_mr_regattr(ofi_domain, &attr, flags, &mr_);
    if (ret != 0) {
        std::ostringstream oss;
        oss << "ofi_mr_raii: fi_mr_regattr failed with RC: " << ret
            << ", Error: " << fi_strerror(-ret);
        NCCL_OFI_WARN("%s", oss.str().c_str());
        throw std::runtime_error(oss.str());
    }

    NCCL_OFI_TRACE(NCCL_NET, "Created MR wrapper with fid_mr: %p", mr_);
}

// Move constructor
ofi_mr_raii::ofi_mr_raii(ofi_mr_raii &&other) noexcept
    : mr_(other.mr_)
{
    other.mr_ = nullptr;
    NCCL_OFI_TRACE(NCCL_NET, "Moved MR wrapper, fid_mr: %p", mr_);
}

// Move assignment operator
ofi_mr_raii &ofi_mr_raii::operator=(ofi_mr_raii &&other) noexcept
{
    if (this != &other) {
        // Clean up current resource if we have one
        if (mr_) {
            NCCL_OFI_TRACE(NCCL_NET, "Closing MR in move assignment, fid_mr: %p", mr_);
            int ret = fi_close(&mr_->fid);
            if (ret != 0) {
                NCCL_OFI_WARN("Failed to close MR in move assignment. RC: %d, Error: %s",
                              ret, fi_strerror(-ret));
            }
        }

        // Transfer ownership
        mr_ = other.mr_;
        other.mr_ = nullptr;

        NCCL_OFI_TRACE(NCCL_NET, "Move assigned MR wrapper, fid_mr: %p", mr_);
    }
    return *this;
}

// Destructor - automatically closes the memory region
ofi_mr_raii::~ofi_mr_raii() noexcept
{
    if (mr_) {
        NCCL_OFI_TRACE(NCCL_NET, "Destroying MR wrapper, closing fid_mr: %p", mr_);
        int ret = fi_close(&mr_->fid);
        if (ret != 0) {
            NCCL_OFI_WARN("Failed to close MR in destructor. RC: %d, Error: %s",
                          ret, fi_strerror(-ret));
        }
    }
}

// Bind memory region to endpoint (for endpoint_mr providers)
int ofi_mr_raii::bind_ep(std::shared_ptr<ofi_ep_raii> ep, uint64_t flags)
{
    if (!ep) {
        NCCL_OFI_WARN("Attempted to bind null EP to MR");
        return -FI_EINVAL;
    }

    return bind(&ep->get()->fid, flags);
}

// Bind memory region to endpoint
int ofi_mr_raii::bind(struct fid *fid, uint64_t flags) const
{
    if (!mr_) {
        NCCL_OFI_WARN("Attempted to bind invalid MR");
        return -FI_EINVAL;
    }

    int ret = fi_mr_bind(mr_, fid, flags);
    if (ret != 0) {
        NCCL_OFI_TRACE(NCCL_NET, "fi_mr_bind returned error: %d (%s)",
                       ret, fi_strerror(-ret));
    }

    return ret;
}

// Enable memory region
int ofi_mr_raii::enable() const
{
    if (!mr_) {
        NCCL_OFI_WARN("Attempted to enable invalid MR");
        return -FI_EINVAL;
    }

    int ret = fi_mr_enable(mr_);
    if (ret != 0) {
        NCCL_OFI_TRACE(NCCL_NET, "fi_mr_enable returned error: %d (%s)",
                       ret, fi_strerror(-ret));
    }

    return ret;
}

// Get memory region key
uint64_t ofi_mr_raii::key() const noexcept
{
    if (!mr_) {
        NCCL_OFI_WARN("Attempted to get key from invalid MR");
        return FI_KEY_NOTAVAIL;
    }

    return fi_mr_key(mr_);
}

// Get memory region descriptor
void *ofi_mr_raii::desc() const noexcept
{
    if (!mr_) {
        NCCL_OFI_WARN("Attempted to get descriptor from invalid MR");
        return nullptr;
    }

    return fi_mr_desc(mr_);
}

// Get direct access to the underlying fid_mr
struct fid_mr *ofi_mr_raii::get() const noexcept
{
    return mr_;
}

// Arrow operator for direct access to fid_mr members
struct fid_mr *ofi_mr_raii::operator->() const noexcept
{
    return mr_;
}

// Check if the MR is valid
bool ofi_mr_raii::is_valid() const noexcept
{
    return mr_ != nullptr;
}
