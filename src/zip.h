/* This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0.  See the enclosed file LICENSE for a copy or if
 * that was not distributed with this file, You can obtain one at
 * http://mozilla.org/MPL/2.0/.
 *
 * Copyright 2017 Max H. Gerlach
 * 
 * */

/*
 * zip.h
 *
 *  Created on: Feb 18, 2013
 *      Author: gerlach
 */

#ifndef ZIP_H_
#define ZIP_H_

#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wconversion"
#pragma GCC diagnostic ignored "-Wshadow"
#include <boost/iterator/zip_iterator.hpp>
#include <boost/range.hpp>
#pragma GCC diagnostic pop

//zip iterators over several sequences to allow iterating over all of them in one go;
//will iterate over tuples of elements from the different sequences
// taken from
// http://stackoverflow.com/questions/8511035/sequence-zip-function-for-c11

template <typename... T>
auto zip(const T&... containers)
-> boost::iterator_range<boost::zip_iterator<decltype(
        boost::make_tuple(std::begin(containers)...))>>
{
    auto zip_begin = boost::make_zip_iterator(boost::make_tuple(std::begin(containers)...));
    auto zip_end = boost::make_zip_iterator(boost::make_tuple(std::end(containers)...));
    return boost::make_iterator_range(zip_begin, zip_end);
}

#endif /* ZIP_H_ */
