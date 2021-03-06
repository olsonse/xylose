/*==============================================================================
 * Public Domain Contributions 2010 United States Government                   *
 * as represented by the U.S. Air Force Research Laboratory.                   *
 *                                                                             *
 * This file is part of xylose                                                 *
 *                                                                             *
 * This program is free software: you can redistribute it and/or modify it     *
 * under the terms of the GNU Lesser General Public License as published by    *
 * the Free Software Foundation, either version 3 of the License, or (at your  *
 * option) any later version.                                                  *
 *                                                                             *
 * This program is distributed in the hope that it will be useful, but WITHOUT *
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or       *
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public        *
 * License for more details.                                                   *
 *                                                                             *
 * You should have received a copy of the GNU Lesser General Public License    *
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.       *
 *                                                                             *
 -----------------------------------------------------------------------------*/

/*! 
  \file ThreadedExecutor.hpp

  \brief Thread cache APPSPACK executor.

  \sa \ref pageCustomize_executor 
*/

/** \example src/xylose/fit/appspack/appsPack.cpp
 * The stand-alone threaded appspack driver also serves as a great example for
 * using the xylose::fit::appspack::ThreadedExecutor.
 */


#ifndef xylose_fit_appspack_ThreadedExecutor_hpp
#define xylose_fit_appspack_ThreadedExecutor_hpp

#include <xylose/strutil.h>
#include <xylose/PThreadCache.h>

#include <appspack/APPSPACK_Executor_Interface.hpp> // Abstract interface for class

namespace xylose {
  namespace fit {
    namespace appspack {

      using xylose::pthreadCache;

      /** Custom multi-threaded executor class derived from APPSPACK::Executor::Interface
       * @see pageCustomize_executor 
       */
      template < typename MinFunc >
      class ThreadedExecutor : public APPSPACK::Executor::Interface {
        /* TYPEDEFS */
      private:
        struct Task : xylose::PThreadTask {
          /* MEMBER STORAGE */
          /** Reference to MinFunc functor object. */
          const MinFunc & minFunc;

          /** MinFunc function input. */
          const APPSPACK::Vector x;

          /** MinFunc function output. */
          APPSPACK::Vector f;

          /** Task tag. */
          const int tag;

          /** Success/Error message. */
          std::string msg_out;



          /* MEMBER FUNCTIONS */
          /** APPSPACK Task constructor. */
          Task( const MinFunc & minFunc,
                const APPSPACK::Vector & x,
                const int & tag )
            : minFunc(minFunc), x(x), f(), tag(tag) { }

          /** APPSPACK Task virtual destructor. */
          virtual ~Task() { }

          /** Execute the APPSPACK MinFunc function. */
          virtual void exec() {
            msg_out = minFunc( x.getStlVector(), f, tag,
                               to_string( pthread_self() ) );

            if (f.size() > 0 && msg_out.size() == 0)
              msg_out = "success";
            else if (f.size() == 0 && msg_out.size() == 0)
              msg_out = "unknown error";
          }/* exec() */
        };

        typedef xylose::PThreadTaskSet PThreadTaskSet;


        /* MEMBER STORAGE */
      private:
        /** Reference to MinFunc functor object. */
        const MinFunc & minFunc;

        PThreadTaskSet queued_tasks;
        PThreadTaskSet finished_tasks;




        /* MEMBER FUNCTIONS */
      public:
        /** Constuctor for ThreadExecutor. */
        ThreadedExecutor( const MinFunc & minFunc)
          : minFunc(minFunc), queued_tasks(), finished_tasks() { }

        /** Destuctor for ThreadExecutor. */
        virtual ~ThreadedExecutor() { }

        /** Returns true if there is a worker free; otherwise, returns false.
         * Since this implementation uses a task queue, we'll always return true.
         * */
        virtual bool isWaiting() const { return true; }

        //! Spawns a point on a free worker and returns true if successful; otherwise, returns false
        virtual bool spawn(const APPSPACK::Vector& x_in, int tag_in) {
          Task * t = new Task( minFunc, x_in, tag_in );
          queued_tasks.insert(t);
          pthreadCache.addTask(t);
          return true;
        }

        /*!
          Checks to see if there is a result from a worker. Returns 0 if no
          worker has returned a message. Otherwise, it returns the worker
          id (or any value greater than zero) and fills in the
          information. It's critical that the tag_out be filled in so that
          the function value can be matched with the correct
          point. Moreover, the msg_out should contain some sort of message
          even for successful evaluations, e.g., "success".
        */
        virtual int recv(int& tag_out, APPSPACK::Vector& f_out, std::string& msg_out) {
          Task * t = NULL;

          if ( finished_tasks.size() > 0 ) {
            t = static_cast<Task*>( * finished_tasks.begin() );
            finished_tasks.erase( t );

          } else if ( queued_tasks.size() > 0 ) {
            PThreadTaskSet finished = pthreadCache.waitForTasks(queued_tasks);
            {
              /* let's make sure that we remove the finished tasks from the queue
               * list */
              PThreadTaskSet tmp;
              std::set_difference(queued_tasks.begin(), queued_tasks.end(),
                                  finished.begin(), finished.end(),
                                  inserter(tmp, tmp.begin()));
              queued_tasks.swap(tmp);
            }

            t = static_cast<Task*>( * finished.begin() );
            finished.erase( t );
            /* now add the remaining items to the finished_tasks array for later
             * retrieval. */
            finished_tasks.insert( finished.begin(), finished.end() );

          } else {
            msg_out = "no tasks to complete";
            return 0;
          }


          tag_out = t->tag;
          f_out = t->f;
          msg_out = t->msg_out;

          /* we can now free the task memory. */
          delete t;

          return 1;
        }

        //! Print information about the executor. 
        /*! Typically called in the intialization of the solver.*/
        // virtual void print() const;

      };

    }/* namespace xylose::fit::appspack */
  }/* namespace xylose::fit */
}/* namespace xylose */

#endif // xylose_fit_appspack_ThreadedExecutor_hpp
