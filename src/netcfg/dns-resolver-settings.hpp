//  OpenVPN 3 Linux client -- Next generation OpenVPN client
//
//  Copyright (C) 2018         OpenVPN, Inc. <sales@openvpn.net>
//  Copyright (C) 2018         David Sommerseth <davids@openvpn.net>
//
//  This program is free software: you can redistribute it and/or modify
//  it under the terms of the GNU Affero General Public License as
//  published by the Free Software Foundation, version 3 of the
//  License.
//
//  This program is distributed in the hope that it will be useful,
//  but WITHOUT ANY WARRANTY; without even the implied warranty of
//  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
//  GNU Affero General Public License for more details.
//
//  You should have received a copy of the GNU Affero General Public License
//  along with this program.  If not, see <https://www.gnu.org/licenses/>.
//

/**
 * @file   dns-resolver-settings.hpp
 *
 * @brief  Generic interface for manipulate DNS resolver settings, regardless
 *         of the resolver configuration approach is used on the system
 */


#pragma once

#include <openvpn/common/rc.hpp>

#include "netcfg-exception.hpp"

using namespace openvpn;

namespace OpenVPN3
{
namespace NetCfg
{
namespace DNS
{
    /**
     *  Class to provide a structured way to manage resolver settings,
     *  such as DNS name servers and DNS search domains
     */
    class ResolverSettings : public RC<thread_unsafe_refcount>
    {
    public:
        typedef RCPtr<ResolverSettings> Ptr;

        ResolverSettings()
        {
        }

        virtual ~ResolverSettings()
        {
        }


        ResolverSettings(const ResolverSettings& origin)
        {
            dns_servers = origin.dns_servers;
            dns_search = origin.dns_search;
        }



        /**
         *  Get indication if the resolver settings has been modified
         *  since the last update/apply.
         *
         *  @returns Returns true if there are un-applied changes
         */
        virtual bool GetModified()
        {
            return modified;
        }


        /**
         *  Trigger fetching the current resolver settings from the
         *  resolver backend.  This must be implemented for each backend.
         */
        virtual void Fetch() = 0;


        /**
         *  The updates the resolver backend with the settings stored
         *  by this object.  This must be implemented for each backend.
         */
        virtual void Apply() = 0;


        /**
         *  Restores the previous resolver settings on the system.
         *  This must be implemented for each backend.
         */
        virtual void Restore() = 0;


        /**
         *  Clear the list of DNS name servers
         */
        virtual void ClearDNSServers() noexcept
        {
            dns_servers.clear();
            modified = true;
        }


        /**
         *  Adds a new single DNS name server
         * @param server  std::string of DNS server to enlist
         */
        virtual void AddDNSServer(const std::string& server)
        {
            dns_servers.push_back(server);
            modified = true;
        }


        /**
         *  Removes a single DNS name server
         *
         * @param server  std::string of DNS server to remove
         */
        virtual void RemoveDNSServer(const std::string& server)
        {
            dns_servers.erase(std::remove(dns_servers.begin(),
                                          dns_servers.end(),
                                          server.c_str()),
                              dns_servers.end());
            modified = true;
        }


        /**
         *  Retrieve the current list of DNS name servers
         *
         * @return  Returns a std::vector<std::string> of all registered
         *          DNS name servers
         */
        virtual std::vector<std::string> GetDNSServers() noexcept
        {
            return dns_servers;
        }


        /**
         *  Clears the list of DNS search domains
         */
        virtual void ClearDNSSearch() noexcept
        {
            dns_search.clear();
            modified = true;
        }


        /**
         *  Adds a new DNS search domain
         *
         * @param domain  std::string of the search domain to add
         */
        virtual void AddDNSSearch(const std::string& domain)
        {
            dns_search.push_back(domain);
            modified = true;
        }

        /**
         *  Removes an already resgistered single DNS search domain
         *
         * @param domain  std::string of the search domain to remove
         */
        virtual void RemoveDNSSearch(const std::string& domain)
        {
            dns_search.erase(std::remove(dns_search.begin(),
                                         dns_search.end(),
                                         domain.c_str()),
                                         dns_search.end());
            modified = true;
        }


#ifdef __GIO_TYPES_H__  // Only add GLib/GDBus methods if this is already used
        /**
         *  Adds DNS name servers based on an array of strings provided via
         *  a GVariant container of the (as) type.
         *
         * @param params  GVariant object containing an (as) based string
         *                array of elements to process
         */
        virtual void AddDNSServers(GVariant *params)
        {
            std::string params_type(g_variant_get_type_string(params));
            if ("(as)" != params_type)
            {
                throw NetCfgException("Invalid D-Bus data type");
            }

            GVariantIter *srvlist = nullptr;
            g_variant_get(params, "(as)", &srvlist);
            if (nullptr == srvlist)
            {
                throw NetCfgException("Failed to extract parameters");
            }

            GVariant *srv = nullptr;
            while ((srv = g_variant_iter_next_value(srvlist)))
            {
                gsize len;
                dns_servers.push_back(std::string(g_variant_get_string(srv, &len)));
                g_variant_unref(srv);
            }
            g_variant_iter_free(srvlist);
            modified = true;
        }


        /**
         *  Removes already resgistered DNS servers based on an array
         *  of strings provided via a GVariant container of the (as) type.
         *
         * @param params  GVariant object containing an (as) based string
         *                array of elements to process
         */
        virtual void RemoveDNSServers(GVariant *params)
        {
            std::string params_type(g_variant_get_type_string(params));
            if ("(as)" != params_type)
            {
                throw NetCfgException("Invalid D-Bus data type");
            }

            GVariantIter *srvlist = nullptr;
            g_variant_get(params, "(as)", &srvlist);
            if (nullptr == srvlist)
            {
                throw NetCfgException("Failed to extract parameters");
            }

            GVariant *srv = nullptr;
            while ((srv = g_variant_iter_next_value(srvlist)))
            {
                gsize len;
                std::string s(g_variant_get_string(srv, &len));
                RemoveDNSServer(s);
                g_variant_unref(srv);
            }
            g_variant_iter_free(srvlist);
            modified = true;
        }


        /**
         *  Adds new DNS search domains based on an array of strings provided
         *  via a GVariant container of the (as) type.
         * @param params  GVariant object containing an (as) based string
         *                array of elements to process
         *
         */
        virtual void AddDNSSearch(GVariant *params)
        {
            std::string params_type(g_variant_get_type_string(params));
            if ("(as)" != params_type)
            {
                throw NetCfgException("Invalid D-Bus data type");
            }

            GVariantIter *srchlist = nullptr;
            g_variant_get(params, "(as)", &srchlist);
            if (nullptr == srchlist)
            {
                throw NetCfgException("Failed to extract parameters");
            }

            GVariant *srchdom = nullptr;
            while ((srchdom = g_variant_iter_next_value(srchlist)))
            {
                gsize len;
                dns_search.push_back(std::string(g_variant_get_string(srchdom, &len)));
                g_variant_unref(srchdom);
            }
            g_variant_iter_free(srchlist);
            modified = true;
        }


        /**
         *  Removes already resgistered DNS search domains based on an array
         *  of strings provided via a GVariant container of the (as) type.
         *
         * @param params  GVariant object containing an (as) based string
         *                array of elements to process
         */
        virtual void RemoveDNSSearch(GVariant *params)
        {
            std::string params_type(g_variant_get_type_string(params));
            if ("(as)" != params_type)
            {
                throw NetCfgException("Invalid D-Bus data type");
            }

            GVariantIter *srchlist = nullptr;
            g_variant_get(params, "(as)", &srchlist);
            if (nullptr == srchlist)
            {
                throw NetCfgException("Failed to extract parameters");
            }

            GVariant *srchdom = nullptr;
            while ((srchdom = g_variant_iter_next_value(srchlist)))
            {
                gsize len;
                std::string dom(g_variant_get_string(srchdom, &len));
                RemoveDNSSearch(dom);
                g_variant_unref(srchdom);
            }
            g_variant_iter_free(srchlist);
            modified = true;
        }

#endif // __GIO_TYPES_H__


        /**
         *  Retrieve the current list of DNS search domains
         *
         * @return  Returns a std::vector<std::string> of all registered
         *          DNS search domains
         */
        virtual std::vector<std::string> GetDNSSearch() noexcept
        {
            return dns_search;
        }


        bool operator==(const ResolverSettings& cmp) const noexcept
        {
            return (dns_servers == cmp.dns_servers
                    && dns_search == cmp.dns_search);

        }

        bool operator!=(const ResolverSettings& cmp) const noexcept
        {
            return (dns_servers != cmp.dns_servers
                    || dns_search != cmp.dns_search);
        }


    protected:
        std::vector<std::string> dns_servers;
        std::vector<std::string> dns_search;
        bool modified = false;
    };

} // namespace DNS
} // namespace NetCfg
} // namespace OpenVPN3

