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

#include "Function.h"
#include "MethodCallback.h"
#include "RawMethodCallback.h"
#include "Factory.h"
#include "Value.h"

#include <cbang/SmartPointer.h>
#include <cbang/json/NullSink.h>


namespace cb {
  namespace js {
    class Sink : public JSON::NullSink {
      SmartPointer<Factory> factory;
      SmartPointer<Value> root;

      bool closeList;
      bool closeDict;

      int index;
      std::string key;
      std::vector<SmartPointer<Value> > stack;

   public:
      Sink(const SmartPointer<Factory> &factory,
           const SmartPointer<Value> &root = 0);
      Sink(const SmartPointer<Factory> &factory, Value &root);

      const SmartPointer<Value> &getRoot() const {return root;}

      // From JSON::Sink
      using JSON::NullSink::insert;
      using JSON::NullSink::write;

      void close() override;
      void reset(const SmartPointer<Value> &root);
      void reset() override {reset(0);}

      // Element functions
      void writeNull() override;
      void writeBoolean(bool value) override;
      void write(double value) override;
      void write(const std::string &value) override;
      void write(const js::Function &func);
      void write(const SmartPointer<Value> &value);

      // List functions
      void beginList(bool simple = false) override;
      void beginAppend() override;
      void endList() override;

      // Dict functions
      void beginDict(bool simple = false) override;
      void beginInsert(const std::string &key) override;
      void endDict() override;


      template <class T>
      void insert(const Signature &sig, T *obj,
                  typename MethodCallback<T>::member_t member) {
        beginInsert(sig.getName());
        write(Function(new MethodCallback<T>(sig, factory, obj, member)));
      }


      template <class T>
      void insert(const Signature &sig, T *obj,
                  typename RawMethodCallback<T>::member_t member) {
        beginInsert(sig.getName());
        write(Function(new RawMethodCallback<T>(sig, factory, obj, member)));
      }
   };
  }
}
