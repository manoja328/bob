#ifndef _TORCH5SPRO_MVSE_CRITERION_H_
#define _TORCH5SPRO_MVSE_CRITERION_H_

#include "machine/Criterion.h"

namespace Torch
{
        //////////////////////////////////////////////////////////////////////////////////////
	// Torch::MVSECriterion:
	//
	// TODO: doxygen header!
	//////////////////////////////////////////////////////////////////////////////////////

	class MVSECriterion : public Criterion
	{
	public:

		/// Constructor
		MVSECriterion(const int target_size);
		
		/// Destructor
		virtual ~MVSECriterion();

		///////////////////////////////////////////////////////////

		///
		virtual bool 	forward(const DoubleTensor *machine_output, const Tensor *target);

		///////////////////////////////////////////////////////////
	};

}

#endif