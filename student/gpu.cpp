/*!
 * @file
 * @brief This file contains implementation of gpu
 *
 * @author Tomáš Milet, imilet@fit.vutbr.cz
 */

#include <student/gpu.hpp>
#include <cstdio>
#include "fwd.hpp"

//mine
struct Triangle{
      OutVertex points[3];
    };

void clear(GPUMemory&mem,ClearCommand cmd){
  if(cmd.clearColor){
    for(int i = 0; i < mem.framebuffer.width * mem.framebuffer.height;)
    {
        float red = cmd.color.r;
        mem.framebuffer.color[i] = (uint8_t)(red*255.f);
        float green = cmd.color.g;
        mem.framebuffer.color[i+1] = (uint8_t)(green*255.f);
        float blue = cmd.color.b;
        mem.framebuffer.color[i+2] = (uint8_t)(blue*255.f);
        float alpha = cmd.color.a;
        mem.framebuffer.color[i+3] = (uint8_t)(alpha*255.f);
        i += 4;
    }
      
    //...
  }
  // clear the depth and stencil buffers if needed
  if (cmd.clearDepth)
  {
    for(int i = 0; i < mem.framebuffer.width * mem.framebuffer.height;i++)
    {
        mem.framebuffer.depth[i] = cmd.depth;
    }

  }
}


//! [gpu_execute]

/**
 * @brief This function reads color from texture.
 *
 * @param texture texture
 * @param uv uv coordinates
 *
 * @return color 4 floats
 */
glm::vec4 read_texture(Texture const&texture,glm::vec2 uv){
  if(!texture.data)return glm::vec4(0.f);
  auto uv1 = glm::fract(uv);
  auto uv2 = uv1*glm::vec2(texture.width-1,texture.height-1)+0.5f;
  auto pix = glm::uvec2(uv2);
  //auto t   = glm::fract(uv2);
  glm::vec4 color = glm::vec4(0.f,0.f,0.f,1.f);
  for(uint32_t c=0;c<texture.channels;++c)
    color[c] = texture.data[(pix.y*texture.width+pix.x)*texture.channels+c]/255.f;
  return color;
}


void  perspectiveDivision(Triangle &triangle)
{
  triangle.points[0].gl_Position.x =  triangle.points[0].gl_Position.x / triangle.points[0].gl_Position.w; 
  triangle.points[0].gl_Position.y =  triangle.points[0].gl_Position.y / triangle.points[0].gl_Position.w; 
  triangle.points[0].gl_Position.z =  triangle.points[0].gl_Position.z / triangle.points[0].gl_Position.w; 

  triangle.points[1].gl_Position.x =  triangle.points[1].gl_Position.x / triangle.points[1].gl_Position.w; 
  triangle.points[1].gl_Position.y =  triangle.points[1].gl_Position.y / triangle.points[1].gl_Position.w; 
  triangle.points[1].gl_Position.z =  triangle.points[1].gl_Position.z / triangle.points[1].gl_Position.w; 

  triangle.points[2].gl_Position.x =  triangle.points[2].gl_Position.x / triangle.points[2].gl_Position.w; 
  triangle.points[2].gl_Position.y =  triangle.points[2].gl_Position.y / triangle.points[2].gl_Position.w; 
  triangle.points[2].gl_Position.z =  triangle.points[2].gl_Position.z / triangle.points[2].gl_Position.w; 

}

void viewportTransformation(Triangle &triangle,uint32_t width,uint32_t height)
{
  triangle.points[0].gl_Position.x = round((triangle.points[0].gl_Position.x * 0.5 + 0.5)  * width);
  triangle.points[0].gl_Position.y = round((triangle.points[0].gl_Position.y * 0.5 + 0.5)  * height); 
  triangle.points[1].gl_Position.x = round((triangle.points[1].gl_Position.x * 0.5 + 0.5)  * width);
  triangle.points[1].gl_Position.y = round((triangle.points[1].gl_Position.y * 0.5 + 0.5)  * height); 
  triangle.points[2].gl_Position.x = round((triangle.points[2].gl_Position.x * 0.5 + 0.5)  * width);
  triangle.points[2].gl_Position.y = round((triangle.points[2].gl_Position.y * 0.5 + 0.5)  * height); 
}




void rasterize(Frame&frame,Triangle & triangle,Program const& prg,DrawCommand cmd,GPUMemory &mem,int triangleID)
{


    glm::ivec2 maxXY(
        glm::min(static_cast<int>(frame.width) - 1, static_cast<int>(std::ceil(glm::max(triangle.points[0].gl_Position.x, glm::max(triangle.points[1].gl_Position.x, triangle.points[2].gl_Position.x))))),
        glm::min(static_cast<int>(frame.height) - 1, static_cast<int>(std::ceil(glm::max(triangle.points[0].gl_Position.y, glm::max(triangle.points[1].gl_Position.y, triangle.points[2].gl_Position.y))))));

    glm::ivec2 minXY(
        glm::max(0, static_cast<int>(std::floor(glm::min(triangle. points[0].gl_Position.x, glm::min(triangle.points[1].gl_Position.x, triangle. points[2].gl_Position.x))))),
        glm::max(0, static_cast<int>(std::floor(glm::min(triangle. points[0].gl_Position.y, glm::min(triangle.points[1].gl_Position.y, triangle. points[2].gl_Position.y))))));




      glm::vec3 hrana1 = triangle.points[1].gl_Position - triangle. points[0].gl_Position;
      glm::vec3 hrana2 = triangle.points[2].gl_Position - triangle. points[0].gl_Position;
      glm::vec3 cross_product = glm::cross(hrana1, hrana2);

      // BAckface culling
      if (cross_product.z <= 0 && cmd.backfaceCulling) {
          return;
      }

    glm::vec3 cords[3];
    cords[1] = triangle.points[1].gl_Position;
    cords[2] = triangle.points[2].gl_Position;  
    cords[0] = triangle.points[0].gl_Position;

    ShaderInterface tempShaderInterface;
    tempShaderInterface.uniforms = &mem.uniforms[0];


    for (int y = minXY.y; y <= maxXY.y; ++y)
    {
          for (int x = minXY.x; x <= maxXY.x; ++x) 
          {
            
            float divisor = ( (cords[0].x - cords[2].x) * (cords[1].y - cords[2].y) +  (cords[0].y - cords[2].y) * (cords[2].x - cords[1].x));
            glm::vec2 pixel_center = glm::vec2(x + 0.5f, y + 0.5f);

            
            float a=( (pixel_center.x - cords[2].x) * (cords[1].y - cords[2].y) +  (pixel_center.y - cords[2].y) * (cords[2].x - cords[1].x)) / divisor;

            float b=( (pixel_center.x - cords[2].x) * (cords[2].y - cords[0].y) +  (pixel_center.y - cords[2].y) * (cords[0].x - cords[2].x) ) / divisor;

            float c = 1 - a - b;

            float c2=triangle. points[2]. gl_Position.w;

            float c0=triangle. points[0]. gl_Position.w;

            float c1=triangle. points[1]. gl_Position.w;



            glm::vec3 falseBarCords( c0*a,c1* b, c2*c);

            glm::vec3 rightBarCords(
              falseBarCords.x /triangle.points[0].gl_Position.w,
              falseBarCords.y /triangle.points[1].gl_Position.w,
              falseBarCords.z /triangle.points[2].gl_Position.w);

            float barSum = rightBarCords.z + rightBarCords.x + rightBarCords.y ;

            rightBarCords /= barSum;

            float z2 = triangle .points[2]. gl_Position.z/triangle .points[2].gl_Position.w;
            float z0 = triangle .points[0]. gl_Position.z/triangle .points[0].gl_Position.w;
            float z1 = triangle .points[1]. gl_Position.z/triangle .points[1].gl_Position.w;

            float newDepth = rightBarCords.x*z0  +
              rightBarCords.y*z1  +
              rightBarCords.z * z2;


            if (rightBarCords.z >= 0 && rightBarCords.x >= 0 && rightBarCords.y >= 0 )
            {
                InFragment inFragment;
                inFragment.gl_FragCoord = glm::vec4(pixel_center, newDepth, 1);

                inFragment.attributes[0].u1 = triangleID;


                inFragment.attributes[0].v4 = glm::vec4(triangleID,
                                                      *reinterpret_cast<float*>(&mem.uniforms[0].u1),
                                                      *reinterpret_cast<float*>(&mem.uniforms[0].i1),
                                                      mem.uniforms[0].v1);


                for (int i = 0; i < 3; ++i)
                {
                    inFragment.attributes[i].v4 = (triangle.points[0].attributes[i].v4*rightBarCords.x/triangle.points[0]. gl_Position.w) +
                                                  (triangle.points[1].attributes[i].v4*rightBarCords.y/triangle.points[1]. gl_Position.w) +
                                                  (triangle.points[2].attributes[i].v4*rightBarCords.z/triangle.points[2]. gl_Position.w);
                    inFragment.attributes[i].v4 /= (rightBarCords.x/triangle.points[0]. gl_Position.w +
                                                    rightBarCords.y/triangle.points[1]. gl_Position.w +
                                                    rightBarCords.z/triangle.points[2]. gl_Position.w);
                }


                  OutFragment outFragment;
                  mem.programs[0].fragmentShader(outFragment, inFragment, tempShaderInterface);




                
                    int depthIdx = static_cast<int>(y * frame.width + x);

                    bool DepthUpdate = newDepth <= frame.depth[depthIdx] && outFragment.gl_FragColor.a > 0.5f;

                    if (DepthUpdate)
                    {
                      frame.depth[depthIdx] = newDepth;
                    }


                    if (newDepth <= frame.depth[depthIdx] && outFragment.gl_FragColor.a > 0.5f)
                    {
                        frame.depth[depthIdx] = newDepth;

                        int idx = depthIdx * frame.channels;
                      
                        
                        float a = outFragment.gl_FragColor.a;

                        glm::vec4 ColorFrame(
                            static_cast<float>(frame.color[idx + 0]),
                            static_cast<float>(frame.color[idx + 1]),
                            static_cast<float>(frame.color[idx + 2]),
                            static_cast<float>(frame.color[idx + 3]));

                        glm::vec3 frameColoor = glm::vec3(ColorFrame.r, ColorFrame.g, ColorFrame.b) / 255.f;
                        glm::vec3 ColorOut = glm::clamp(frameColoor * (1 - a) + glm::vec3(outFragment.gl_FragColor) * a, glm::vec3(0.f), glm::vec3(1.f)) * 255.f;

                        if (DepthUpdate)
                        {
                            frame.color[idx + 0] = static_cast<uint8_t>(ColorOut.r);
                            frame.color[idx + 1] = static_cast<uint8_t>(std::round(ColorOut.g));
                            frame.color[idx + 2] = static_cast<uint8_t>(std::round(ColorOut.b));
                            frame.color[idx + 3] = static_cast<uint8_t>(ColorFrame.a);
                        }
                  }


          }
         }
       }

}

InVertex runVertexAssembly(InVertex inVertex,VertexArray ver_arr,GPUMemory&mem,int i,DrawCommand cmd){
  //################ VERTEX ID ################
  if(cmd.vao.indexBufferID == -1)
    {
      inVertex.gl_VertexID = i;
    }
    else
    {
      if(cmd.vao.indexType == IndexType::UINT8)
      {
        uint8_t* vertex_index;
        vertex_index = (uint8_t *)(mem.buffers[cmd.vao.indexBufferID].data + cmd.vao.indexOffset + i);
        inVertex.gl_VertexID =(uint32_t)(*vertex_index);
      }
      else if(cmd.vao.indexType == IndexType::UINT16)
      {
        uint16_t* vertex_index;
        vertex_index = (uint16_t *)(mem.buffers[cmd.vao.indexBufferID].data + cmd.vao.indexOffset + i*2);
        inVertex.gl_VertexID =(uint32_t)(*vertex_index);
      }
      else
      {
        uint32_t* vertex_index;
        vertex_index = (uint32_t *)(mem.buffers[cmd.vao.indexBufferID].data + cmd.vao.indexOffset + i*4);
        inVertex.gl_VertexID =*vertex_index;
      }


    }
  //################ VERTEX ID END ################


  for (int j = 0; j < maxAttributes; j++)
  {
    VertexAttrib attrib = ver_arr.vertexAttrib[j];
    Buffer indexBuffer = mem.buffers[ver_arr.vertexAttrib[j].bufferID];
    switch (ver_arr.vertexAttrib[j].type )
    {
    case AttributeType::EMPTY:
      break;
    case AttributeType::FLOAT:
        inVertex.attributes[j].v1 = *(float*)(indexBuffer.data + attrib.offset + attrib.stride * inVertex.gl_VertexID);
        break;
    case AttributeType::VEC2:
        inVertex.attributes[j].v2 = *(glm::vec2*)(indexBuffer.data + attrib.offset + attrib.stride * inVertex.gl_VertexID);

        break;
    case AttributeType::VEC3:
        inVertex.attributes[j].v3 = *(glm::vec3*)(indexBuffer.data + attrib.offset + attrib.stride * inVertex.gl_VertexID);
        break;
    case AttributeType::VEC4:
        inVertex.attributes[j].v4 = *(glm::vec4*)(indexBuffer.data + attrib.offset + attrib.stride * inVertex.gl_VertexID);
        break;
    case AttributeType::UINT:
        inVertex.attributes[j].v1 = *(uint32_t*)(indexBuffer.data + attrib.offset + attrib.stride * inVertex.gl_VertexID);
        break;
    case AttributeType::UVEC2:
        inVertex.attributes[j].v2 = *(glm::uvec2*)(indexBuffer.data + attrib.offset + attrib.stride * inVertex.gl_VertexID);

        break;
    case AttributeType::UVEC3:
        inVertex.attributes[j].v3 = *(glm::uvec3*)(indexBuffer.data + attrib.offset + attrib.stride * inVertex.gl_VertexID);
        break;
    case AttributeType::UVEC4:
        inVertex.attributes[j].v4 = *(glm::uvec4*)(indexBuffer.data + attrib.offset + attrib.stride * inVertex.gl_VertexID);
        break;
    default:
      break;
    }
    
  }
  return inVertex;

}

// //okipirovane z vystupu, predtim tu nebyla
void drawImpl(DrawCommand cmd,GPUMemory &mem,OutVertex *outVertexArray,Program prg){
      static int triangleId = 0;
      triangleId++;

      //LOAD TRIANGLE
      Triangle triangle;
      triangle.points[0] = outVertexArray[0];
      triangle.points[1] = outVertexArray[1];
      triangle.points[2] = outVertexArray[2];

      perspectiveDivision(triangle);

      viewportTransformation(triangle,mem.framebuffer.width,mem.framebuffer.height);

      rasterize(mem.framebuffer,triangle,mem.programs[cmd.programID],cmd,mem,triangleId);

    }

void draw(GPUMemory&mem,DrawCommand cmd,uint32_t draw_command_num){
  
  Program prg = mem.programs[cmd.programID];

  OutVertex outVertArray[3];

  for(int i = 0; i < cmd.nofVertices;i++){
    InVertex inVertex;
    inVertex.gl_DrawID = draw_command_num;
    inVertex = runVertexAssembly(inVertex,cmd.vao,mem,i,cmd);
    OutVertex outVertex;
    ShaderInterface si;
    prg.vertexShader(outVertex,inVertex,si);
    outVertArray[i%3] = outVertex;  
    if(i%3==2)
    {
      drawImpl(cmd,mem,outVertArray,prg); 
    }
  }

}

//! [gpu_execute]
void gpu_execute(GPUMemory&mem,CommandBuffer &cb){

  /// \todo Tato funkce reprezentuje funkcionalitu grafické karty.<br>
  /// Měla by umět zpracovat command buffer, čistit framebuffer a kresli.<br>
  /// mem obsahuje paměť grafické karty.
  /// cb obsahuje command buffer pro zpracování.
  /// Bližší informace jsou uvedeny na hlavní stránce dokumentace.
  uint32_t draw_commands_count = 0;


  for(uint32_t i=0;i<cb.nofCommands;++i)
  {
    CommandType type = cb.commands[i].type;
    CommandData data = cb.commands[i].data;
    if(type == CommandType::CLEAR)
    {
      clear(mem,data.clearCommand);
    }
    if (type == CommandType::DRAW )
    {
      draw(mem, data.drawCommand,draw_commands_count);
      draw_commands_count ++;
    }
  }
}