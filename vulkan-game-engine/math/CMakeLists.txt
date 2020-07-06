
set( MATH_SOURCE Matrix.cpp; Spline.cpp; Vector.cpp; Quaternion.cpp )
set( MATH_HEADER Matrix.hpp; Spline.hpp; Vector.hpp; Quaternion.hpp )

add_library( mathlib OBJECT
	${MATH_HEADER}
	${MATH_SOURCE}
)