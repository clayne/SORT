/*
    This file is a part of SORT(Simple Open Ray Tracing), an open-source cross
    platform physically based renderer.
 
    Copyright (c) 2011-2018 by Cao Jiayin - All rights reserved.
 
    SORT is a free software written for educational purpose. Anyone can distribute
    or modify it under the the terms of the GNU General Public License Version 3 as
    published by the Free Software Foundation. However, there is NO warranty that
    all components are functional in a perfect manner. Without even the implied
    warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
    General Public License for more details.
 
    You should have received a copy of the GNU General Public License along with
    this program. If not, see <http://www.gnu.org/licenses/gpl-3.0.html>.
 */

#include "operation_node.h"

IMPLEMENT_CREATOR( SORTNodeOneMinus );
IMPLEMENT_CREATOR( AddNode );
IMPLEMENT_CREATOR( LerpNode );
IMPLEMENT_CREATOR( BlendNode );
IMPLEMENT_CREATOR( MutiplyNode );
IMPLEMENT_CREATOR( GammaToLinearNode );
IMPLEMENT_CREATOR( LinearToGammaNode );
IMPLEMENT_CREATOR( NormalDecoderNode );

bool OperatorNode::CheckValidation()
{
    m_node_valid = MaterialNode::CheckValidation();
    
    for( auto input : m_props ){
        auto type = input.second->node ? input.second->node->getNodeType() : MAT_NODE_CONSTANT;
        if( type & MAT_NODE_BXDF ){
            m_node_valid = false;
            break;
        }
    }
    
    return m_node_valid;
}

// Adding node
AddNode::AddNode()
{
	REGISTER_MATERIALNODE_PROPERTY( "Color1" , src0 );
	REGISTER_MATERIALNODE_PROPERTY( "Color2" , src1 );
}

// get property value
MaterialPropertyValue AddNode::GetNodeValue( Bsdf* bsdf )
{
	return src0.GetPropertyValue(bsdf) + src1.GetPropertyValue(bsdf);
}

// inverse node
SORTNodeOneMinus::SORTNodeOneMinus()
{
    REGISTER_MATERIALNODE_PROPERTY( "Color" , src );
}

// get property value
MaterialPropertyValue SORTNodeOneMinus::GetNodeValue( Bsdf* bsdf )
{
    return MaterialPropertyValue(1.0f) - src.GetPropertyValue(bsdf);
}

LerpNode::LerpNode()
{
	REGISTER_MATERIALNODE_PROPERTY( "Color1" , src0 );
	REGISTER_MATERIALNODE_PROPERTY( "Color2" , src1 );
	REGISTER_MATERIALNODE_PROPERTY( "Factor" , factor );
}

// update bsdf
void LerpNode::UpdateBSDF( Bsdf* bsdf , Spectrum weight )
{
	if( weight.IsBlack() )
		return;

	float f = factor.GetPropertyValue(bsdf).x;

	if( src0.node )
		src0.node->UpdateBSDF( bsdf, weight * ( 1.0f - f ) );
	if( src1.node )
		src1.node->UpdateBSDF( bsdf, weight * f );
}

// get property value
MaterialPropertyValue LerpNode::GetNodeValue( Bsdf* bsdf )
{
	float f = factor.GetPropertyValue( bsdf ).x;
	return src0.GetPropertyValue(bsdf) * ( 1.0f - f ) + src1.GetPropertyValue(bsdf) * f;
}

BlendNode::BlendNode()
{
	REGISTER_MATERIALNODE_PROPERTY( "Color1" , src0 );
	REGISTER_MATERIALNODE_PROPERTY( "Color2" , src1 );
	REGISTER_MATERIALNODE_PROPERTY( "Factor1" , factor0 );
	REGISTER_MATERIALNODE_PROPERTY( "Factor2" , factor1 );
}

// update bsdf
void BlendNode::UpdateBSDF( Bsdf* bsdf , Spectrum weight )
{
	if( weight.IsBlack() )
		return;

	float f0 = factor0.GetPropertyValue(bsdf).x;
	float f1 = factor1.GetPropertyValue(bsdf).y;
	if( src0.node )
		src0.node->UpdateBSDF( bsdf, weight * f0 );
	if( src1.node )
		src1.node->UpdateBSDF( bsdf, weight * f1 );
}

// get property value
MaterialPropertyValue BlendNode::GetNodeValue( Bsdf* bsdf )
{
	float f0 = factor0.GetPropertyValue( bsdf ).x;
	float f1 = factor1.GetPropertyValue( bsdf ).x;
	return src0.GetPropertyValue(bsdf) * f0 + src1.GetPropertyValue(bsdf) * f1;
}

MutiplyNode::MutiplyNode()
{
	REGISTER_MATERIALNODE_PROPERTY( "Color1" , src0 );
	REGISTER_MATERIALNODE_PROPERTY( "Color2" , src1 );
}

// update bsdf
void MutiplyNode::UpdateBSDF( Bsdf* bsdf , Spectrum weight )
{
	if( weight.IsBlack() )
		return;

	MAT_NODE_TYPE type0 = (src0.node)?src0.node->getNodeType():MAT_NODE_CONSTANT;
	MAT_NODE_TYPE type1 = (src1.node)?src1.node->getNodeType():MAT_NODE_CONSTANT;

	if( type0 & MAT_NODE_BXDF )
		src0.node->UpdateBSDF( bsdf , weight * src1.GetPropertyValue(bsdf).x );
	else if( type1 & MAT_NODE_BXDF )
		src1.node->UpdateBSDF( bsdf , weight * src0.GetPropertyValue(bsdf).x );
}

// get property value
MaterialPropertyValue MutiplyNode::GetNodeValue( Bsdf* bsdf )
{
	return src0.GetPropertyValue(bsdf) * src1.GetPropertyValue(bsdf);
}

GammaToLinearNode::GammaToLinearNode()
{
    REGISTER_MATERIALNODE_PROPERTY( "Color" , src );
}

// get property value
MaterialPropertyValue GammaToLinearNode::GetNodeValue( Bsdf* bsdf )
{
    auto tmp = src.GetPropertyValue(bsdf);
    tmp.x = GammaToLinear(tmp.x);
    tmp.y = GammaToLinear(tmp.y);
    tmp.z = GammaToLinear(tmp.z);
    return tmp;
}

LinearToGammaNode::LinearToGammaNode()
{
    REGISTER_MATERIALNODE_PROPERTY( "Color" , src );
}

// get property value
MaterialPropertyValue LinearToGammaNode::GetNodeValue( Bsdf* bsdf )
{
    auto tmp = src.GetPropertyValue(bsdf);
    tmp.x = LinearToGamma(tmp.x);
    tmp.y = LinearToGamma(tmp.y);
    tmp.z = LinearToGamma(tmp.z);
    return tmp;
}

NormalDecoderNode::NormalDecoderNode()
{
    REGISTER_MATERIALNODE_PROPERTY( "Color" , src );
}

// get property value
MaterialPropertyValue NormalDecoderNode::GetNodeValue( Bsdf* bsdf )
{
    auto tmp = src.GetPropertyValue(bsdf);
    return MaterialPropertyValue( 2.0f * tmp.x - 1.0f , tmp.z , 2.0f * tmp.y - 1.0f , 0.0f );
}
