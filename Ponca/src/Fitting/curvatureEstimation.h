﻿/*
 This Source Code Form is subject to the terms of the Mozilla Public
 License, v. 2.0. If a copy of the MPL was not distributed with this
 file, You can obtain one at http://mozilla.org/MPL/2.0/.
*/

#pragma once

#include "./defines.h"

namespace Ponca
{

template < class DataPoint, class _WFunctor, typename T>
/**
 *
 * \brief Base class for any 3d curvature estimator: holds k1, k2 and associated vectors
 *
 * \ingroup fitting
 */
class BaseCurvatureEstimator : public T
{
private:
    typedef T Base;

protected:
    enum
    {
        PROVIDES_PRINCIPALE_CURVATURES
    };

public:
    typedef typename Base::Scalar          Scalar;      /*!< \brief Inherited scalar type*/
    typedef typename Base::VectorType      VectorType;  /*!< \brief Inherited vector type*/
    typedef typename DataPoint::MatrixType MatrixType;  /*!< \brief Matrix type inherited from DataPoint*/

protected:
    /// \brief Principal curvature with highest absolute magnitude
    Scalar m_k1,
    /// \brief Principal curvature with smallest absolute magnitude
           m_k2;
    /// \brief Direction associated to the principal curvature with highest absolute magnitude
    VectorType m_v1,
    /// \brief Direction associated to the principal curvature with highest smallest magnitude
               m_v2;

    static_assert ( DataPoint::Dim == 3, "BaseCurvatureEstimator is only valid in 3D");

public:
    /*! \brief Default constructor */
    PONCA_MULTIARCH inline BaseCurvatureEstimator() : m_k1(0), m_k2(0) {}

public:
    /**************************************************************************/
    /* Initialization                                                         */
    /**************************************************************************/
    /*! \copydoc Concept::FittingProcedureConcept::init() */
    PONCA_MULTIARCH inline void init (const VectorType& _evalPos);

    /**************************************************************************/
    /* Use results                                                            */
    /**************************************************************************/
    //! \brief Returns an estimate of the first principal curvature value
    //!
    //! It is the greatest curvature in <b>absolute value</b>.
    PONCA_MULTIARCH inline Scalar k1() const { return m_k1; }

    //! \brief Returns an estimate of the second principal curvature value
    //!
    //! It is the smallest curvature in <b>absolute value</b>.
    PONCA_MULTIARCH inline Scalar k2() const { return m_k2; }

    //! \brief Returns an estimate of the first principal curvature direction
    //!
    //! It is the greatest curvature in <b>absolute value</b>.
    PONCA_MULTIARCH inline VectorType k1Direction() const { return m_v1; }

    //! \brief Returns an estimate of the second principal curvature direction
    //!
    //! It is the smallest curvature in <b>absolute value</b>.
    PONCA_MULTIARCH inline VectorType k2Direction() const { return m_v2; }

    //! \brief Returns an estimate of the mean curvature
    PONCA_MULTIARCH inline Scalar kMean() const { return (m_k1 + m_k2)/2.;}

    //! \brief Returns an estimate of the Gaussian curvature
    PONCA_MULTIARCH inline Scalar GaussianCurvature() const { return m_k1 * m_k2;}
};


/*!
 * \brief Extension to compute curvature values based on a covariance analysis
 * of normal vectors of neighbors.
 *
 * A 3D covariance matrix is computed from the normals of the neighbors and the
 * two principal curvature values and directions are given by the two extreme
 * eigenvalues and associated eigenvectors of the covariance matrix
 * \cite Liang:1990:RRSS.
 *
 * \warning This class is valid only in 3D.
 * \ingroup fitting
 */
template < class DataPoint, class _WFunctor, typename T>
class NormalCovarianceCurvature : public BaseCurvatureEstimator<DataPoint,_WFunctor,T>
{
private:
    typedef BaseCurvatureEstimator<DataPoint,_WFunctor,T> Base;

    //TODO(thib) check the curvature values that might be wrong
    //TODO(thib) use weighting function
    //TODO(thib) which eigenvectors should be selected ? extreme of maximal ?

public:
    typedef typename Base::Scalar          Scalar;      /*!< \brief Inherited scalar type*/
    typedef typename Base::VectorType      VectorType;  /*!< \brief Inherited vector type*/
    typedef typename DataPoint::MatrixType MatrixType;  /*!< \brief Matrix type inherited from DataPoint*/
    /*! \brief Solver used to analyse the covariance matrix*/
    typedef Eigen::SelfAdjointEigenSolver<MatrixType> Solver;

protected:
    MatrixType m_cov;   /*!< \brief Covariance matrix */
    VectorType m_cog;   /*!< \brief Gravity center */
    Solver m_solver;    /*!< \brief Solver used to analyse the covariance matrix */

public:
    /*! \brief Default constructor */
    PONCA_MULTIARCH inline NormalCovarianceCurvature() : Base() {}

public:
    /**************************************************************************/
    /* Initialization                                                         */
    /**************************************************************************/
    /*! \copydoc Concept::FittingProcedureConcept::init() */
    PONCA_MULTIARCH inline void init (const VectorType& _evalPos);

    /**************************************************************************/
    /* Processing                                                             */
    /**************************************************************************/
    /*! \copydoc Concept::FittingProcedureConcept::addNeighbor() */
    PONCA_MULTIARCH inline bool addNeighbor(const DataPoint &_nei);

    /*! \copydoc Concept::FittingProcedureConcept::finalize() */
    PONCA_MULTIARCH inline FIT_RESULT finalize();
};


/*!
 * \brief Extension to compute curvature values based on a covariance analysis
 * of normal vectors of neighbors projected onto the tangent plane.
 *
 * A 2D covariance matrix is computed from the projections of normals of the
 * neighbors and the two principal curvature values and directions are given by
 * the eigenvalues and associated eigenvectors of the covariance matrix
 * \cite Berkmann:1994:CSG.
 *
 * \note This procedure requires two passes, the first one for plane fitting
 * and local frame estimation, and the second one for covariance analysis.
 * \warning This class is valid only in 3D.
 * \ingroup fitting
 */
template < class DataPoint, class _WFunctor, typename T>
class ProjectedNormalCovarianceCurvature : public BaseCurvatureEstimator<DataPoint,_WFunctor,T>
{
private:
    typedef BaseCurvatureEstimator<DataPoint,_WFunctor,T> Base;

    //TODO(thib) check the curvature values that might be wrong
    //TODO(thib) use weighting function

protected:
    enum
    {
        Check = Base::PROVIDES_PLANE
    };

    enum PASS : int
    {
        FIRST_PASS = 0,
        SECOND_PASS,
        PASS_COUNT
    };

public:
    typedef typename Base::Scalar          Scalar;      /*!< \brief Inherited scalar type*/
    typedef typename Base::VectorType      VectorType;  /*!< \brief Inherited vector type*/
    typedef typename DataPoint::MatrixType MatrixType;  /*!< \brief Matrix type inherited from DataPoint*/
    //TODO(thib) use of Eigen::RowAtCompileTime-1 ?
    typedef Eigen::Matrix<Scalar,2,2> Mat22;
    typedef Eigen::Matrix<Scalar,3,2> Mat32;
    typedef Eigen::Matrix<Scalar,2,1> Vector2;
    typedef typename VectorType::Index Index;
    /*! \brief Solver used to analyse the covariance matrix*/
    typedef Eigen::SelfAdjointEigenSolver<Mat22> Solver;

protected:
    Vector2 m_cog;      /*!< \brief Gravity center */
    Mat22 m_cov;        /*!< \brief Covariance matrix */
    Solver m_solver;    /*!< \brief Solver used to analyse the covariance matrix */
    PASS m_pass;        /*!< \brief Current pass */
    Mat32 m_tframe;     /*!< \brief Tangent frame */

public:
    /**************************************************************************/
    /* Initialization                                                         */
    /**************************************************************************/
    /*! \copydoc Concept::FittingProcedureConcept::init() */
    PONCA_MULTIARCH inline void init (const VectorType& _evalPos);

    /**************************************************************************/
    /* Processing                                                             */
    /**************************************************************************/
    /*! \copydoc Concept::FittingProcedureConcept::addNeighbor() */
    PONCA_MULTIARCH inline bool addNeighbor(const DataPoint &_nei);

    /*! \copydoc Concept::FittingProcedureConcept::finalize() */
    PONCA_MULTIARCH inline FIT_RESULT finalize();

};

#include "curvatureEstimation.hpp"

} //namespace Ponca
