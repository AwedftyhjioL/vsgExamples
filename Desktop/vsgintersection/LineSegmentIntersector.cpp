/* <editor-fold desc="MIT License">

Copyright(c) 2020 Robert Osfield

Permission is hereby granted, free of charge, to any person obtaining a copy of this software and associated documentation files (the "Software"), to deal in the Software without restriction, including without limitation the rights to use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the Software is furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.

</editor-fold> */

#include "LineSegmentIntersector.h"

#include <iostream>

using namespace vsg;

struct TriangleIntersector
{
    using value_type = float;

    TriangleIntersector(const vec3& s, const vec3& e)
    {
        start = s;
        end = e;

        _d = e - s;
        _length = length(_d);
        _inverse_length = (_length!=0.0) ? 1.0/_length : 0.0;
        _d *= _inverse_length;

        _d_invX = _d.x!=0.0 ? _d/_d.x : vec3(0.0, 0.0, 0.0);
        _d_invY = _d.y!=0.0 ? _d/_d.y : vec3(0.0, 0.0, 0.0);
        _d_invZ = _d.z!=0.0 ? _d/_d.z : vec3(0.0, 0.0, 0.0);
    }

    /// intersect with a single triangle
    bool intersect(const vec3& v0, const vec3& v1, const vec3& v2);

    vec3 start;
    vec3 end;

    vec3        _d;
    value_type  _length;
    value_type  _inverse_length;

    vec3        _d_invX;
    vec3        _d_invY;
    vec3        _d_invZ;
};

bool TriangleIntersector::intersect(const vec3& v0, const vec3& v1, const vec3& v2)
{
    // const vec3& ls = startend.first;
    // const vec3& le = startend.second;

    vec3 T = start - v0;
    vec3 E2 = v2 - v0;
    vec3 E1 = v1 - v0;

    vec3 P =  cross(_d , E2);

    value_type det = dot(P, E1);

    value_type r, r0, r1, r2;

    const value_type epsilon = 1e-10;
    if (det>epsilon)
    {
        value_type u = dot(P, T);
        if (u<0.0 || u>det) return false;

        vec3 Q = cross(T , E1);
        value_type v = dot(Q, _d);
        if (v<0.0 || v>det) return false;

        if ((u+v)> det) return false;

        value_type inv_det = 1.0/det;
        value_type t = dot(Q, E2)*inv_det;
        if (t<0.0 || t>_length) return false;

        u *= inv_det;
        v *= inv_det;

        r0 = 1.0-u-v;
        r1 = u;
        r2 = v;
        r = t * _inverse_length;
    }
    else if (det<-epsilon)
    {
        value_type u = dot(P, T);
        if (u>0.0 || u<det) return false;

        vec3 Q = cross(T, E1);
        value_type v = dot(Q, _d);
        if (v>0.0 || v<det) return false;

        if ((u+v) < det) return false;

        value_type inv_det = 1.0/det;
        value_type t = dot(Q, E2)*inv_det;
        if (t<0.0 || t>_length) return false;

        u *= inv_det;
        v *= inv_det;

        r0 = 1.0-u-v;
        r1 = u;
        r2 = v;
        r = t * _inverse_length;
    }
    else
    {
        return false;
    }

    // TODO : handle hit

    std::cout<<"We have a hit"<<std::endl;
    return true;
}

LineSegmentIntersector::LineSegmentIntersector(const dvec3& s, const dvec3& e) :
    start(s),
    end(e) {}

ref_ptr<Intersector> LineSegmentIntersector::transform(const dmat4& m)
{
    auto transformed = LineSegmentIntersector::create(m * start, m * end);
    return transformed;
}

bool LineSegmentIntersector::intersects(const dsphere& bs)
{
    //std::cout<<"intersects( center = "<<bs.center<<", radius = "<<bs.radius<<")"<<std::endl;
    if (!bs.valid()) return false;

    dvec3 sm = start - bs.center;
    double c = length2(sm) - bs.radius * bs.radius;
    if (c<0.0) return true;

    dvec3 se = end-start;
    double a = length2(se);
    double b = dot(sm, se)*2.0;
    double d = b*b-4.0*a*c;

    if (d<0.0) return false;

    d = sqrt(d);

    double div = 1.0/(2.0*a);

    double r1 = (-b-d)*div;
    double r2 = (-b+d)*div;

    if (r1<=0.0 && r2<=0.0) return false;
    if (r1>=1.0 && r2>=1.0) return false;

    // passed all the rejection tests so line must intersect bounding sphere, return true.
    return true;
}

bool LineSegmentIntersector::intersect(VkPrimitiveTopology topology, const vsg::DataList& arrays, uint32_t firstVertex, uint32_t vertexCount)
{
    // TODO
    std::cout<<"LineSegmentIntersector::intersects( topology "<<topology<<", arrays.size() = "<<arrays.size()<<" firstVertex = "<<firstVertex<<", vertexCount = "<<vertexCount<<")"<<std::endl;
    return false;
}

bool LineSegmentIntersector::intersect(VkPrimitiveTopology topology, const vsg::DataList& arrays, vsg::ref_ptr<vsg::Data> indices, uint32_t firstIndex, uint32_t indexCount)
{
    // TODO
    //std::cout<<"LineSegmentIntersector::intersects( topology "<<topology<<", arrays.size() = "<<arrays.size()<<", indices = "<<indices.get()<<" firstIndex = "<<firstIndex<<", indexCount = "<<indexCount<<")"<<std::endl;
    if (arrays.empty() || !indices || indexCount==0) return false;

    auto vertices = arrays[0].cast<vec3Array>();
    auto us_indices = indices.cast<ushortArray>();

    if (!vertices || !us_indices) return false;

    //std::cout<<"   "<<vertices->size()<<", us_indices ="<<us_indices->size()<<std::endl;
    size_t previous_size = intersections.size();

    uint32_t endIndex = firstIndex + indexCount;
    switch(topology)
    {
        case(VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST) :
        {
            vec3 f_start(start);
            vec3 f_end(end);
            TriangleIntersector triIntsector(f_start, f_end);
            for(uint32_t i = firstIndex; i<endIndex; i+=3)
            {
                triIntsector.intersect( vertices->at(us_indices->at(i)), vertices->at(us_indices->at(i+1)), vertices->at(us_indices->at(i+2)));
            }
            break;
        }
        default: break; // TODO: NOT supported
    }

    return intersections.size() != previous_size;
}
