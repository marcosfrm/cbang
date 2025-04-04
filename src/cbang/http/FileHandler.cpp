/******************************************************************************\

          This file is part of the C! library.  A.K.A the cbang library.

                Copyright (c) 2021-2025, Cauldron Development  Oy
                Copyright (c) 2003-2021, Cauldron Development LLC
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

#include "FileHandler.h"
#include "Request.h"

#include <cbang/event/Buffer.h>
#include <cbang/os/SystemUtilities.h>
#include <cbang/log/Logger.h>

using namespace std;
using namespace cb;
using namespace cb::HTTP;


FileHandler::FileHandler(const JSON::ValuePtr &config) :
  FileHandler(config->getString("path"), config->getU32("prefix", 0),
    config->getString("index", "")) {}


FileHandler::FileHandler(const string &root, unsigned pathPrefix,
  const string &index) :
  root(root), pathPrefix(pathPrefix), index(index),
  directory(SystemUtilities::isDirectory(root)) {}


bool FileHandler::operator()(Request &req) {
  string path;

  if (directory) {
    string orig = req.getURI().getPath();
    if (orig.length() <= pathPrefix) return false;
    orig = orig.substr(pathPrefix);

    // Remove unsafe parts
    vector<string> parts;
    String::tokenize(orig, parts, "/");
    vector<string> result;

    for (auto &part: parts) {
      if (part == ".") continue;
      if (part == "..") {
        if (result.empty()) THROWX("Invalid path", HTTP_UNAUTHORIZED);
        result.pop_back();

      } else result.push_back(part);
    }

    // Relative to root
    path = SystemUtilities::joinPath(root, String::join(result, "/"));

    // Handle index
    if (!index.empty() && SystemUtilities::isDirectory(path))
      path += "/" + index;

  } else path = root; // Single file

  LOG_INFO(5, "FileHandler() " << path);

  if (!SystemUtilities::isFile(path)) return false;

  // Send file
  Event::Buffer buf;
  buf.addFile(path);
  req.reply(buf);

  return true;
}
