/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2003-2024, Cauldron Development LLC
                               All rights reserved.

         The C! library is free software: you can redistribute it and/or
        modify it under the terms of the GNU Lesser General Public License
       as published by the Free Software Foundation, either version 2.1 of
               the License, or (at your option) any later version.

        The C! library is distributed in the hope that it will be useful,
          but WITHOUT ANY WARRANTY; without even the implied warranty of
        MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
                 Lesser General Public License for more details.

         You should have received a copy of the GNU Lesser General Public
                 License along with the C! library.  If not, see
                         <http://www.gnu.org/licenses/>.

        In addition, BSD licensing may be granted on a case by case basis
        by written permission from at least one of the copyright holders.
           You may request written permission by emailing the authors.

                  For information regarding this software email:
                                 Joseph Coffland
                          joseph@cauldrondevelopment.com

\******************************************************************************/

#include "AddressRange.h"

#include <cbang/Exception.h>
#include <cbang/String.h>
#include <cbang/log/Logger.h>

#include <cctype>
#include <utility>
#include <algorithm>

using namespace std;
using namespace cb;


AddressRange::AddressRange(const string &spec) {
  try {
    auto dash = spec.find('-');

    if (dash != string::npos) {
      start = SockAddr::parse(spec.substr(0, dash));
      end   = SockAddr::parse(spec.substr(dash + 1));
      if (end < start) swap(start, end);

    } else {
      auto slash = spec.find('/');

      if (slash != string::npos) {
        uint8_t bits = String::parseU8(spec.substr(slash + 1), true);
        start = end = SockAddr::parse(spec.substr(0, slash));
        start.setCIDRBits(bits, 0);
        end  .setCIDRBits(bits, 1);

      } else start = end = SockAddr::parse(spec);
    }

  } catch (const Exception &e) {
    THROWC("Invalid IP address range '" << spec << "'", e);
  }
}


AddressRange::AddressRange(const SockAddr &start, const SockAddr &end) {
  if (start < end) {this->start = start; this->end = end;}
  else {this->start = end; this->end = start;}
}


string AddressRange::toString() const {
  if (start == end) return start.toString();

  auto bits = getCIDRBits();
  if (bits) return start.toString() + String::printf("/%u", bits);

  return start.toString() + "-" + end.toString();
}


int AddressRange::cmp(const SockAddr &addr) const {
  return addr < start ? -1 : (end < addr ? 1 : 0);
}


bool AddressRange::overlaps(const AddressRange &o) const {
  return contains(o.start) || contains(o.end) || o.contains(end);
}


bool AddressRange::adjacent(const SockAddr &addr) const {
  return addr.adjacent(start) || addr.adjacent(end);
}


bool AddressRange::adjacent(const AddressRange &o) const {
  return o.end.adjacent(start) || end.adjacent(o.start);
}


void AddressRange::add(const AddressRange &o) {
  if (!adjacent(o) && !overlaps(o))
    THROW("Ranges are not adjacent and do not overlap");

  if (o.start < start) start = o.start;
  if (end     < o.end) end   = o.end;
}