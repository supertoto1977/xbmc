/*
 *  Copyright (C) 2005-2018 Team Kodi
 *  This file is part of Kodi - https://kodi.tv
 *
 *  SPDX-License-Identifier: LGPL-2.1-or-later
 *  See LICENSES/README.md for more information.
 */

#pragma once

#include <array>
#include <cmath>
#include <memory>

extern "C" {
#include <libavutil/pixfmt.h>
}

template<uint8_t Order>
class CMatrix
{
public:
  CMatrix() = default;
  explicit CMatrix(const CMatrix<Order - 1>& other);
  explicit CMatrix(const std::array<std::array<float, Order>, Order>& other) { m_mat = other; }
  explicit CMatrix(const std::array<std::array<float, Order - 1>, Order - 1>& other);
  virtual ~CMatrix() = default;

  virtual CMatrix operator*(const std::array<std::array<float, Order>, Order>& other);

  CMatrix operator*(const CMatrix& other);
  CMatrix& operator*=(const CMatrix& other);

  CMatrix& operator=(const std::array<std::array<float, Order - 1>, Order - 1>& other);

  bool operator==(const CMatrix<Order>& other) const
  {
    for (int i = 0; i < Order; ++i)
    {
      for (int j = 0; j < Order; ++j)
      {
        if (m_mat[i][j] == other.m_mat[i][j])
          continue;

        // some floating point comparisions should be done by checking if the difference is within a tolerance
        if (std::abs(m_mat[i][j] - other.m_mat[i][j]) <=
            (std::max(std::abs(other.m_mat[i][j]), std::abs(m_mat[i][j])) * 1e-2f))
          continue;

        return false;
      }
    }

    return true;
  }

  std::array<float, Order>& operator[](int index) { return m_mat[index]; }

  const std::array<float, Order>& operator[](int index) const { return m_mat[index]; }

  std::array<std::array<float, Order>, Order>& Get();

  const std::array<std::array<float, Order>, Order>& Get() const;

  CMatrix& Invert();

  float* ToRaw() { return &m_mat[0][0]; }

protected:
  std::array<std::array<float, Order>, Order> Invert(
      std::array<std::array<float, Order>, Order>& other) const;

  std::array<std::array<float, Order>, Order> m_mat{{}};
};

class CGlMatrix : public CMatrix<4>
{
public:
  CGlMatrix() = default;
  explicit CGlMatrix(const CMatrix<3>& other);
  explicit CGlMatrix(const std::array<std::array<float, 3>, 3>& other);
  ~CGlMatrix() override = default;
  CMatrix operator*(const std::array<std::array<float, 4>, 4>& other) override;
};

class CScale : public CGlMatrix
{
public:
  CScale(float x, float y, float z);
  ~CScale() override = default;
};

class CTranslate : public CGlMatrix
{
public:
  CTranslate(float x, float y, float z);
  ~CTranslate() override = default;
};

class ConversionToRGB : public CMatrix<3>
{
public:
  ConversionToRGB(float Kr, float Kb);
  ~ConversionToRGB() override = default;

protected:
  ConversionToRGB() = default;

  float a11, a12, a13;
  float CbDen, CrDen;
};

class PrimaryToXYZ : public CMatrix<3>
{
public:
  PrimaryToXYZ(const float (&primaries)[3][2], const float (&whitepoint)[2]);
  ~PrimaryToXYZ() override = default;

protected:
  PrimaryToXYZ() = default;
  float CalcBy(const float p[3][2], const float w[2]);
  float CalcGy(const float p[3][2], const float w[2], const float By);
  float CalcRy(const float By, const float Gy);
};

class PrimaryToRGB : public PrimaryToXYZ
{
public:
  PrimaryToRGB(float (&primaries)[3][2], float (&whitepoint)[2]);
  ~PrimaryToRGB() override = default;
};

//------------------------------------------------------------------------------

using Matrix4 = CMatrix<4>;
using Matrix3 = CMatrix<3>;
using Matrix3x1 = std::array<float, 3>;

class CConvertMatrix
{
public:
  CConvertMatrix() = default;
  ~CConvertMatrix() = default;

  CConvertMatrix& SetSourceColorSpace(AVColorSpace colorSpace);
  CConvertMatrix& SetSourceBitDepth(int bits);
  CConvertMatrix& SetSourceLimitedRange(bool limited);
  CConvertMatrix& SetSourceTextureBitDepth(int textureBits);
  CConvertMatrix& SetSourceColorPrimaries(AVColorPrimaries src);

  CConvertMatrix& SetDestinationColorPrimaries(AVColorPrimaries dst);
  CConvertMatrix& SetDestinationContrast(float contrast);
  CConvertMatrix& SetDestinationBlack(float black);
  CConvertMatrix& SetDestinationLimitedRange(bool limited);

  Matrix4 GetYuvMat();
  Matrix3 GetPrimMat();
  float GetGammaSrc();
  float GetGammaDst();

  static Matrix3x1 GetRGBYuvCoefs(AVColorSpace colspace);

private:
  const CGlMatrix& GenMat();
  const CMatrix<3>& GenPrimMat();

  std::unique_ptr<CGlMatrix> m_mat;
  std::unique_ptr<CMatrix<3>> m_matPrim;

  AVColorSpace m_colSpace = AVCOL_SPC_BT709;
  AVColorPrimaries m_colPrimariesSrc = AVCOL_PRI_BT709;
  float m_gammaSrc = 2.2f;
  bool m_limitedSrc = true;
  AVColorPrimaries m_colPrimariesDst = AVCOL_PRI_BT709;
  float m_gammaDst = 2.2f;
  bool m_limitedDst = false;
  int m_srcBits = 8;
  int m_srcTextureBits = 8;
  int m_dstBits = 8;
  float m_contrast = 1.0;
  float m_black = 0.0;
};
