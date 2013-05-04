#ifndef CPPLIB_VEC2_H
#define CPPLIB_VEC2_H
/** \file
 * \brief Definition of simple vector in 2-dimension.
 *
 * It also declares friend operators that enable vector arithmetics of arbitrary order.
 */
#include <assert.h>
#include <math.h>

namespace cpplib{

/** \brief Vector in 2-dimension with arbitrary elements.
 *
 * It internally use an array to store the values for making
 * repeating processes easy.
 *
 * There's no SIMD or such hardware-specific optimization.
 * I don't feel like being burdened by heavy-weight linear
 * algebra library right now.
 */
template<typename T> class Vec2{
	T a[2];
public:
	typedef Vec2<T> tt; ///< This Type
	typedef T intype[2]; ///< Internal type.

	/// The default constructor does nothing to allocated memory area, which means uninitialized object.
	tt(){}

	/// Initialize from an array of size 3.
	tt(const T o[2]){a[0] = o[0], a[1] = o[1];}

	/// Specify individual elements.
	tt(T x, T y){a[0] = x, a[1] = y;}

	/// Assigns zero to all elements.
	void clear(){a[0] = a[1] = 0;}

	/// Adds given vector into this object.
	tt &addin(const tt &o);

	/// Calculates sum of two vectors.
	tt add(const tt &o)const;

	/// Calculates length of this vector.
	/// \return Length (norm) of this vector.
	T len()const{
		return ::sqrt(a[0] * a[0] + a[1] * a[1]);
	}

	/// \brief Calculates squared length of this vector.
	///
	/// This operation is faster than len() because no square root is required.
	/// \return Squared length (norm) of this vector.
	T slen()const{
		return a[0] * a[0] + a[1] * a[1];
	}

	/// Creates a normalized version of this vector.
	/// \return Newly created vector with the same direction of this and length of 1.
	tt norm()const{
		double s = len();
		return tt(a[0]/s,a[1]/s);
	}

	/// Normalizes this vector. \return This vector.
	tt &normin(){
		double s = len(); a[0] /= s, a[1] /= s;
		return *this;
	}

	/// Unary minus operator inverts elements.
	/// \return Newly created vector with negated elements of this.
	tt operator-()const{return tt(-a[0], -a[1]);}

	/// Adds given vector into this object. Same as addin().
	tt &operator+=(const tt &o){return addin(o);}

	/// Calculates sum of two vectors. Same as add().
	tt operator+(const tt &o)const{return add(o);}

	/// Subtracts this vector by given vector. Same as addin(-o).
	tt &operator-=(const tt &o){return addin(-o);}

	/// Calculates difference of two vectors. Same as add(-o).
	tt operator-(const tt &o)const{return add(-o);}

	/// Scale this vector by given scalar.
	/// \return This vector
	tt &scalein(T s){
		a[0] *= s, a[1] *= s;
		return *this;
	}

	/// Calculate scaled version of this vector by given scalar.
	/// \return Newly created vector
	tt scale(T s)const{
		tt ret;
		((ret.a)[0]=(a)[0]*(s),(ret.a)[1]=(a)[1]*(s));
		return ret;
	}

	/// Calculate scalar (dot) product with another vector.
	T sp(const tt &o)const{
		return a[0] * o.a[0] + a[1] * o.a[1];
	}

	/// Calculate vector (cross) product with another vector.
	T cross(const tt &o)const{
		return (a[1]*o.a[2]-a[2]*o.a[1]);
	}

	/// scalein().
	tt &operator*=(const T s){return scalein(s);}

	/// scale().
	tt operator*(const T s)const{return scale(s);}

	/// \brief Divide by a scalar.
	tt &operator/=(const T s);

	/// \brief Create divided version of this by given scalar.
	tt operator/(const T s)const;

	/// Binary multiply operator with a scalar as the first argument need to be global function.
	friend tt operator*(const T s, const tt &o){return o * s;}

	/// Cast to array of T.
	operator T*(){return a;} operator const T*()const{return a;}

	/// Compares two vectors.
	bool operator==(const tt &o)const{return a[0] == o.a[0] && a[1] == o.a[1];}

	/// Compares two vectors.
	bool operator!=(const tt &o)const{return !operator==(o);}

	/// cast to T* do the job, but range is not checked.
	T operator[](int i)const{assert(0 <= i && i < 2); return a[i];}
	T &operator[](int i){assert(0 <= i && i < 2); return a[i];}

	/// cast to T(*[3]) do not cope with cast to T*, so here is a unary operator to explicitly do this.
	intype *operator ~(){return &a;} const intype *operator ~()const{return &a;}

	/// Converting elements to a given type requires explicit call.
	template<class T2> Vec2<T2> cast()const{return Vec2<T2>(static_cast<T2>(a[0]),static_cast<T2>(a[1]));}

	/// \brief Array to Class pointer converter.
	///
	/// Use this if you want to pretend an array of scalar as a Vec3 without creating temporary object.
	static tt &atoc(T *a){return *reinterpret_cast<tt*>(a);}
};

typedef Vec2<double> Vec2d; ///< Type definition for double vector.
typedef Vec2<float> Vec2f; ///< Type definition for float vector.
typedef Vec2<int> Vec2i; ///< Type definition for int vector.



//-----------------------------------------------------------------------------
// Implementation
//-----------------------------------------------------------------------------


/// \param o The other object to add.
/// \return This object after addition is performed.
template<typename T>
inline Vec2<T> &Vec2<T>::addin(const Vec2<T> &o){
	a[0] += o.a[0], a[1] += o.a[1];
	return *this;
}

/// \param o The other object to add.
/// \return Newly created object of sum of this and o.
template<typename T>
inline Vec2<T> Vec2<T>::add(const Vec2<T> &o)const{
	return tt(a[0] + o.a[0], a[1] + o.a[1]);
}

/// Division is not equivalent to scale with inverse in case of integral component type.
template<typename T>
inline Vec2<T> &Vec2<T>::operator/=(const T s){
	a[0] /= s, a[1] /= s; return *this;
}

/// Division is not equivalent to scale with inverse in case of integral component type.
template<typename T>
inline Vec2<T> Vec2<T>::operator/(const T s)const{
	return Vec3<T>(a[0] / s, a[1] / s);
}

}

using namespace cpplib;

#endif
