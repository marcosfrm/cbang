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

#pragma once

#include "ColumnDef.h"
#include "Statement.h"

#include <cbang/SmartPointer.h>
#include <cbang/util/StringMap.h>

#include <vector>


namespace cb {
  namespace DB {
    class Database;

    class TableDef {
      std::string dbName;
      std::string name;
      std::string constraints;

      typedef std::vector<ColumnDef> columns_t;
      columns_t columns;

      typedef std::map<std::string, unsigned, StringILess> columnMap_t;
      columnMap_t columnMap;

    public:
      TableDef(const std::string &name,
               const std::string &constraints = std::string()) :
        name(name), constraints(constraints) {}

      static std::string getEscapedName(const std::string &name);
      static std::string getEscapedDBName(const std::string &dbName);
      static std::string getEscapedName(const std::string &dbName,
                                        const std::string &name);
      std::string getEscapedDBName() const {return getEscapedDBName(dbName);}
      std::string getEscapedName() const {return getEscapedName(dbName, name);}

      const std::string &getDBName() const {return dbName;}
      void setDBName(const std::string &dbName) {this->dbName = dbName;}

      const std::string &getName() const {return name;}
      void setName(const std::string &name) {this->name = name;}

      const std::string &getConstraints() const {return constraints;}
      void setConstraints(const std::string &constraints)
      {this->constraints = constraints;}

      void add(const ColumnDef &column);

      unsigned getIndex(const std::string &column) const;

      unsigned getNumColumns() const {return columns.size();}
      typedef columns_t::const_iterator iterator;
      iterator begin() const {return columns.begin();}
      iterator end() const {return columns.end();}

      void create(Database &db) const;
      typedef CIStringMap columnRemap_t;
      void rebuild(Database &db,
                   const columnRemap_t &columnRemap = columnRemap_t()) const;
      void deleteAll(Database &db) const;

      SmartPointer<Statement>
      makeWriteStmt(Database &db,
                    const std::string &suffix = std::string()) const;
      SmartPointer<DB::Statement>
      makeReadStmt(DB::Database &db,
                   const std::string &suffix = std::string()) const;
    };
  }
}
