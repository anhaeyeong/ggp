#include "Renderer/Renderable.h"

#include "assimp/Importer.hpp"	// C++ importer interface
#include "assimp/scene.h"		// output data structure
#include "assimp/postprocess.h"	// post processing flags

#include "Texture/DDSTextureLoader.h"

namespace library
{

    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderable::Renderable

      Summary:  Constructor

      Args:     const XMFLOAT4& outputColor
                  Default color to shader the renderable

      Modifies: [m_vertexBuffer, m_indexBuffer, m_constantBuffer,
                 m_normalBuffer, m_aMeshes, m_aMaterials, m_vertexShader,
                 m_pixelShader, m_outputColor, m_world, m_bHasNormalMap
                 m_aNormalData].
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    /*--------------------------------------------------------------------
      TODO: Renderable::Renderable definition (remove the comment)
    --------------------------------------------------------------------*/
    Renderable::Renderable(_In_ const XMFLOAT4& outputColor)
        : m_outputColor(outputColor)
        , m_world(XMMatrixIdentity())
        , m_vertexBuffer(nullptr)
        , m_indexBuffer(nullptr)
        , m_constantBuffer(nullptr)
        , m_normalBuffer(nullptr)
        , m_vertexShader(nullptr)
        , m_pixelShader(nullptr)
        , m_padding()
        , m_bHasNormalMap(FALSE)
    {

    }

    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderable::initialize

      Summary:  Initializes the buffers and the world matrix

      Args:     ID3D11Device* pDevice
                  The Direct3D device to create the buffers
                ID3D11DeviceContext* pImmediateContext
                  The Direct3D context to set buffers
                PCWSTR pszTextureFileName
                  File name of the texture to usen

      Modifies: [m_vertexBuffer, m_normalBuffer, m_indexBuffer
                 m_constantBuffer].

      Returns:  HRESULT
                  Status code
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    /*--------------------------------------------------------------------
      TODO: Renderable::initialize definition (remove the comment)
    --------------------------------------------------------------------*/
    HRESULT Renderable::initialize(_In_ ID3D11Device* pDevice, _In_ ID3D11DeviceContext* pImmediateContext)
    {
        HRESULT hr = S_OK;

        D3D11_BUFFER_DESC bd = {
        .ByteWidth = GetNumVertices() * sizeof(SimpleVertex),
        .Usage = D3D11_USAGE_DEFAULT,
        .BindFlags = D3D11_BIND_VERTEX_BUFFER,
        .CPUAccessFlags = 0,
        .MiscFlags = 0,
        };

        D3D11_SUBRESOURCE_DATA initData = {
            .pSysMem = getVertices(),
            .SysMemPitch = 0,
            .SysMemSlicePitch = 0
        };
        hr = pDevice->CreateBuffer(&bd, &initData, m_vertexBuffer.GetAddressOf());
        if (FAILED(hr))
        {
            return hr;
        }

        if (HasTexture() && m_aNormalData.empty())
        {
            calculateNormalMapVectors();
        }

        bd.ByteWidth = GetNumVertices() * sizeof(NormalData);
        initData.pSysMem = m_aNormalData.data();
        hr = pDevice->CreateBuffer(&bd, &initData, m_normalBuffer.GetAddressOf());
        if (FAILED(hr))
        {
            return hr;
        }

        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = GetNumIndices() * sizeof(WORD);
        bd.BindFlags = D3D11_BIND_INDEX_BUFFER;
        bd.CPUAccessFlags = 0;

        initData.pSysMem = getIndices();
        hr = pDevice->CreateBuffer(&bd, &initData, m_indexBuffer.GetAddressOf());
        if (FAILED(hr))
        {
            return hr;
        }

        bd.Usage = D3D11_USAGE_DEFAULT;
        bd.ByteWidth = sizeof(CBChangesEveryFrame);
        bd.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
        bd.CPUAccessFlags = 0;
        hr = pDevice->CreateBuffer(&bd, nullptr, m_constantBuffer.GetAddressOf());
        if (FAILED(hr))
        {
            return hr;
        }

        return hr;
    }

    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderable::calculateNormalMapVectors

      Summary:  Calculate tangent and bitangent vectors of every vertex

      Modifies: [m_aNormalData].
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    /*--------------------------------------------------------------------
      TODO: Renderable::calculateNormalMapVectors definition (remove the comment)
    --------------------------------------------------------------------*/
    void Renderable::calculateNormalMapVectors()
    {
        UINT uNumFaces = GetNumIndices() / 3;
        const SimpleVertex* aVertices = getVertices();
        const WORD* aIndices = getIndices();

        m_aNormalData.resize(GetNumVertices(), NormalData());

        XMFLOAT3 tangent, Bitangent;
        for (UINT i = 0u; i < uNumFaces; ++i)
        {
            calculateTangentBitangent(aVertices[aIndices[i * 3]], aVertices[aIndices[i * 3 + 1]], aVertices[aIndices[i * 3 + 2]], tangent, Bitangent);

            m_aNormalData[aIndices[i * 3]].Tangent = tangent;
            m_aNormalData[aIndices[i * 3]].Bitangent = Bitangent;

            m_aNormalData[aIndices[i * 3 + 1]].Tangent = tangent;
            m_aNormalData[aIndices[i * 3 + 1]].Bitangent = Bitangent;

            m_aNormalData[aIndices[i * 3 + 2]].Tangent = tangent;
            m_aNormalData[aIndices[i * 3 + 2]].Bitangent = Bitangent;
        }
    }

    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderable::calculateTangentBitangent

      Summary:  Calculate tangent/bitangent vectors of the given face

      Args:     SimpleVertex& v1
                  The first vertex of the face
                SimpleVertex& v2
                  The second vertex of the face
                SimpleVertex& v3
                  The third vertex of the face
                XMFLOAT3& tangent
                  Calculated tangent vector
                XMFLOAT3& bitangent
                  Calculated bitangent vector
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    void Renderable::calculateTangentBitangent(_In_ const SimpleVertex& v1, _In_ const SimpleVertex& v2, _In_ const SimpleVertex& v3, _Out_ XMFLOAT3& tangent, _Out_ XMFLOAT3& bitangent)
    {
        XMFLOAT3 vector1, vector2;
        XMFLOAT2 tuVector, tvVector;

        // Calculate the two vectors for this face.
        vector1.x = v2.Position.x - v1.Position.x;
        vector1.y = v2.Position.y - v1.Position.y;
        vector1.z = v2.Position.z - v1.Position.z;

        vector2.x = v3.Position.x - v1.Position.x;
        vector2.y = v3.Position.y - v1.Position.y;
        vector2.z = v3.Position.z - v1.Position.z;

        // Calculate the tu and tv texture space vectors.
        tuVector.x = v2.TexCoord.x - v1.TexCoord.x;
        tvVector.x = v2.TexCoord.y - v1.TexCoord.y;

        tuVector.y = v3.TexCoord.x - v1.TexCoord.x;
        tvVector.y = v3.TexCoord.y - v1.TexCoord.y;

        // Calculate the denominator of the tangent/binormal equation.
        float den = 1.0f / (tuVector.x * tvVector.y - tuVector.y * tvVector.x);

        // Calculate the cross products and multiply by the coefficient to get the tangent and binormal.
        tangent.x = (tvVector.y * vector1.x - tvVector.x * vector2.x) * den;
        tangent.y = (tvVector.y * vector1.y - tvVector.x * vector2.y) * den;
        tangent.z = (tvVector.y * vector1.z - tvVector.x * vector2.z) * den;

        bitangent.x = (tuVector.x * vector2.x - tuVector.y * vector1.x) * den;
        bitangent.y = (tuVector.x * vector2.y - tuVector.y * vector1.y) * den;
        bitangent.z = (tuVector.x * vector2.z - tuVector.y * vector1.z) * den;

        // Calculate the length of this normal.
        float length = sqrt((tangent.x * tangent.x) + (tangent.y * tangent.y) + (tangent.z * tangent.z));

        // Normalize the normal and then store it
        tangent.x = tangent.x / length;
        tangent.y = tangent.y / length;
        tangent.z = tangent.z / length;

        // Calculate the length of this normal.
        length = sqrt((bitangent.x * bitangent.x) + (bitangent.y * bitangent.y) + (bitangent.z * bitangent.z));

        // Normalize the normal and then store it
        bitangent.x = bitangent.x / length;
        bitangent.y = bitangent.y / length;
        bitangent.z = bitangent.z / length;
    }


    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderable::AddMaterial

      Summary:  Add material to this renderable

      Args:     std::shared_ptr<Material>& material
                  Material to add

      Modifies: [m_aMaterials]
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    void Renderable::AddMaterial(_In_ const std::shared_ptr<Material>& material)
    {
        m_aMaterials.push_back(material);
    }


    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderable::SetMaterialOfMesh

      Summary:  Set the material of the mesh

      Args:     const UINT uMeshIndex
                  Index of the mesh
                 const UINT uMaterialIndex
                  Index of the material

      Modifies: [m_aMeshes, m_bHasNormalMap]

      Returns:  HRESULT
                  Status code
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    HRESULT Renderable::SetMaterialOfMesh(_In_ const UINT uMeshIndex, _In_ const UINT uMaterialIndex)
    {
        if (uMeshIndex >= m_aMeshes.size() || uMaterialIndex >= m_aMaterials.size())
        {
            return E_FAIL;
        }

        m_aMeshes[uMeshIndex].uMaterialIndex = uMaterialIndex;

        if (m_aMaterials[uMeshIndex]->pNormal)
        {
            m_bHasNormalMap = true;
        }

        return S_OK;
    }


    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderable::GetNormalBuffer

      Summary:  Return the normal buffer

      Returns:  ComPtr<ID3D11Buffer>&
                  Normal buffer
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    /*--------------------------------------------------------------------
      TODO: Renderable::GetNormalBuffer definition (remove the comment)
    --------------------------------------------------------------------*/
    ComPtr<ID3D11Buffer>& Renderable::GetNormalBuffer()
    {
        return m_normalBuffer;
    }

    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderable::GetMaterial

      Summary:  Return the material of the given index

      Args:     UINT uIndex
                  Index of the material

      Returns:  std::shared_ptr<Material>&
                  Material
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    /*--------------------------------------------------------------------
      TODO: Renderable::GetMaterial definition (remove the comment)
    --------------------------------------------------------------------*/
    const std::shared_ptr<Material>& Renderable::GetMaterial(UINT uIndex) const
    {
        assert(uIndex < m_aMaterials.size());

        return m_aMaterials[uIndex];
    }

    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderable::HasNormalMap

      Summary:  Return whether the renderable has normal map

      Returns:  BOOL
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    /*--------------------------------------------------------------------
      TODO: Renderable::HasNormalMap definition (remove the comment)
    --------------------------------------------------------------------*/
    BOOL Renderable::HasNormalMap() const
    {
        return m_bHasNormalMap;
    }
    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderable::SetVertexShader
      Summary:  Sets the vertex shader to be used for this renderable
                object
      Args:     const std::shared_ptr<VertexShader>& vertexShader
                  Vertex shader to set to
      Modifies: [m_vertexShader].
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    /*--------------------------------------------------------------------
      TODO: Renderable::SetVertexShader definition (remove the comment)
    --------------------------------------------------------------------*/
    void Renderable::SetVertexShader(_In_ const std::shared_ptr<VertexShader>& vertexShader)
    {
        m_vertexShader = vertexShader;
    }
    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderable::SetPixelShader
      Summary:  Sets the pixel shader to be used for this renderable
                object
      Args:     const std::shared_ptr<PixelShader>& pixelShader
                  Pixel shader to set to
      Modifies: [m_pixelShader].
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    /*--------------------------------------------------------------------
      TODO: Renderable::SetPixelShader definition (remove the comment)
    --------------------------------------------------------------------*/
    void Renderable::SetPixelShader(_In_ const std::shared_ptr<PixelShader>& pixelShader)
    {
        m_pixelShader = pixelShader;
    }
    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderable::GetVertexShader
      Summary:  Returns the vertex shader
      Returns:  ComPtr<ID3D11VertexShader>&
                  Vertex shader. Could be a nullptr
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    /*--------------------------------------------------------------------
      TODO: Renderable::GetVertexShader definition (remove the comment)
    --------------------------------------------------------------------*/
    ComPtr<ID3D11VertexShader>& Renderable::GetVertexShader()
    {
        return m_vertexShader->VertexShader::GetVertexShader();
    }
    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderable::GetPixelShader
      Summary:  Returns the vertex shader
      Returns:  ComPtr<ID3D11PixelShader>&
                  Pixel shader. Could be a nullptr
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    /*--------------------------------------------------------------------
      TODO: Renderable::GetPixelShader definition (remove the comment)
    --------------------------------------------------------------------*/
    ComPtr<ID3D11PixelShader>& Renderable::GetPixelShader()
    {
        return m_pixelShader->PixelShader::GetPixelShader();
    }
    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderable::GetVertexLayout
      Summary:  Returns the vertex input layout
      Returns:  ComPtr<ID3D11InputLayout>&
                  Vertex input layout
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    /*--------------------------------------------------------------------
      TODO: Renderable::GetVertexLayout definition (remove the comment)
    --------------------------------------------------------------------*/
    ComPtr<ID3D11InputLayout>& Renderable::GetVertexLayout()
    {
        return m_vertexShader->VertexShader::GetVertexLayout();
    }
    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderable::GetVertexBuffer
      Summary:  Returns the vertex buffer
      Returns:  ComPtr<ID3D11Buffer>&
                  Vertex buffer
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    /*--------------------------------------------------------------------
      TODO: Renderable::GetVertexBuffer definition (remove the comment)
    --------------------------------------------------------------------*/
    ComPtr<ID3D11Buffer>& Renderable::GetVertexBuffer()
    {
        return m_vertexBuffer;
    }
    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderable::GetIndexBuffer
      Summary:  Returns the index buffer
      Returns:  ComPtr<ID3D11Buffer>&
                  Index buffer
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    /*--------------------------------------------------------------------
      TODO: Renderable::GetIndexBuffer definition (remove the comment)
    --------------------------------------------------------------------*/
    ComPtr<ID3D11Buffer>& Renderable::GetIndexBuffer()
    {
        return m_indexBuffer;
    }
    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderable::GetConstantBuffer
      Summary:  Returns the constant buffer
      Returns:  ComPtr<ID3D11Buffer>&
                  Constant buffer
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    /*--------------------------------------------------------------------
      TODO: Renderable::GetConstantBuffer definition (remove the comment)
    --------------------------------------------------------------------*/
    ComPtr<ID3D11Buffer>& Renderable::GetConstantBuffer()
    {
        return m_constantBuffer;
    }
    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderable::GetWorldMatrix
      Summary:  Returns the world matrix
      Returns:  const XMMATRIX&
                  World matrix
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    /*--------------------------------------------------------------------
      TODO: Renderable::GetWorldMatrix definition (remove the comment)
    --------------------------------------------------------------------*/
    const XMMATRIX& Renderable::GetWorldMatrix() const
    {
        return m_world;
    }
    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderable::GetOutputColor
      Summary:  Returns the output color
      Returns:  const XMFLOAT4&
                  The output color
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    /*--------------------------------------------------------------------
      TODO: Renderable::GetOutputColor definition (remove the comment)
    --------------------------------------------------------------------*/
    const XMFLOAT4& Renderable::GetOutputColor() const
    {
        return m_outputColor;
    }

    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderable::HasTexture
      Summary:  Returns whether the renderable has texture
      Returns:  BOOL
                  Whether the renderable has texture
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    /*--------------------------------------------------------------------
      TODO: Renderable::HasTexture definition (remove the comment)
    --------------------------------------------------------------------*/
    BOOL Renderable::HasTexture() const
    {
        if (GetNumMaterials() > 0)
        {
            return TRUE;
        }
        return FALSE;
    }

    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderable::GetMesh
      Summary:  Returns a basic mesh entry at given index
      Returns:  const Renderable::BasicMeshEntry&
                  Basic mesh entry at given index
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    const Renderable::BasicMeshEntry& Renderable::GetMesh(UINT uIndex) const
    {
        assert(uIndex < m_aMeshes.size());

        return m_aMeshes[uIndex];
    }

    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderable::RotateX
      Summary:  Rotates around the x-axis
      Args:     FLOAT angle
                  Angle of rotation around the x-axis, in radians
      Modifies: [m_world].
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    /*--------------------------------------------------------------------
      TODO: Renderable::RotateX definition (remove the comment)
    --------------------------------------------------------------------*/
    void Renderable::RotateX(_In_ FLOAT angle)
    {
        // m_world *= x-axis rotation by angle matrix
        m_world *= XMMatrixRotationX(angle);
    }

    void Renderable::RotateY(_In_ FLOAT angle)
    {
        // m_world *= y-axis rotation by angle matrix
        m_world *= XMMatrixRotationY(angle);
    }

    void Renderable::RotateZ(_In_ FLOAT angle)
    {
        // m_world *= z-axis rotation by angle matrix
        m_world *= XMMatrixRotationZ(angle);
    }

    void Renderable::RotateRollPitchYaw(_In_ FLOAT pitch, _In_ FLOAT yaw, _In_ FLOAT roll)
    {
        // m_world *= x, y, z-axis rotation by pitch, yaw, roll matrix
        m_world *= XMMatrixRotationRollPitchYaw(pitch, yaw, roll);
    }

    void Renderable::Scale(_In_ FLOAT scaleX, _In_ FLOAT scaleY, _In_ FLOAT scaleZ)
    {
        // m_world *= x, y, z-axis scaling by scale factor matrix
        m_world *= XMMatrixScaling(scaleX, scaleY, scaleZ);
    }

    void Renderable::Translate(_In_ const XMVECTOR& offset)
    {
        // m_world *= translate by offset vector matrix
        m_world *= XMMatrixTranslationFromVector(offset);
    }

    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderable::GetNumMeshes
      Summary:  Returns the number of meshes
      Returns:  UINT
                  Number of meshes
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    UINT Renderable::GetNumMeshes() const
    {
        return static_cast<UINT>(m_aMeshes.size());
    }

    /*M+M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M+++M
      Method:   Renderable::GetNumMaterials
      Summary:  Returns the number of materials
      Returns:  UINT
                  Number of materials
    M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M---M-M*/
    UINT Renderable::GetNumMaterials() const
    {
        return static_cast<UINT>(m_aMaterials.size());
    }
}
