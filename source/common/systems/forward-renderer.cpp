#include "forward-renderer.hpp"
#include "../mesh/mesh-utils.hpp"
#include "../texture/texture-utils.hpp"

#include <iostream>
using namespace std;
#include "glm/gtx/string_cast.hpp"

namespace our
{

    void ForwardRenderer::initialize(glm::ivec2 windowSize, const nlohmann::json &config)
    {
        // First, we store the window size for later use
        this->windowSize = windowSize;

        // Then we check if there is a sky texture in the configuration
        if (config.contains("sky"))
        {
            // First, we create a sphere which will be used to draw the sky
            this->skySphere = mesh_utils::sphere(glm::ivec2(16, 16));

            // We can draw the sky using the same shader used to draw textured objects
            ShaderProgram *skyShader = new ShaderProgram();
            skyShader->attach("assets/shaders/textured.vert", GL_VERTEX_SHADER);
            skyShader->attach("assets/shaders/textured.frag", GL_FRAGMENT_SHADER);
            skyShader->link();

            // TODO: (Req 10) Pick the correct pipeline state to draw the sky
            //  Hints: the sky will be draw after the opaque objects so we would need depth testing but which depth funtion should we pick?
            //  We will draw the sphere from the inside, so what options should we pick for the face culling.
            PipelineState skyPipelineState{};
            // enable depth testing
            skyPipelineState.depthTesting.enabled = true;
            // put the test function GL_LEQUAL
            // Passes if the fragment's depth value is less than or equal to the stored depth value.
            skyPipelineState.depthTesting.function = GL_LEQUAL;
            // enable faceculling testing
            skyPipelineState.faceCulling.enabled = true;
            // remove the front face
            skyPipelineState.faceCulling.culledFace = GL_FRONT;
            skyPipelineState.faceCulling.frontFace = GL_CCW;

            // Load the sky texture (note that we don't need mipmaps since we want to avoid any unnecessary blurring while rendering the sky)
            std::string skyTextureFile = config.value<std::string>("sky", "");
            Texture2D *skyTexture = texture_utils::loadImage(skyTextureFile, false);

            // Setup a sampler for the sky
            Sampler *skySampler = new Sampler();
            skySampler->set(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            skySampler->set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            skySampler->set(GL_TEXTURE_WRAP_S, GL_REPEAT);
            skySampler->set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            // Combine all the aforementioned objects (except the mesh) into a material
            this->skyMaterial = new TexturedMaterial();
            this->skyMaterial->shader = skyShader;
            this->skyMaterial->texture = skyTexture;
            this->skyMaterial->sampler = skySampler;
            this->skyMaterial->pipelineState = skyPipelineState;
            this->skyMaterial->tint = glm::vec4(1.0f, 1.0f, 1.0f, 1.0f);
            this->skyMaterial->alphaThreshold = 1.0f;
            this->skyMaterial->transparent = false;
        }

        // Then we check if there is a postprocessing shader in the configuration
        if (config.contains("postprocess"))
        {
            // TODO: (Req 11) Create a framebuffer
            // created the frame buffer and binded it to draw framebuffer
            glGenFramebuffers(1, &postprocessFrameBuffer);
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, postprocessFrameBuffer);

            // TODO: (Req 11) Create a color and a depth texture and attach them to the framebuffer
            //  Hints: The color format can be (Red, Green, Blue and Alpha components with 8 bits for each channel).
            //  The depth format can be (Depth component with 24 bits).

            colorTarget = texture_utils::empty(GL_RGBA8, windowSize);
            glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, colorTarget->getOpenGLName(), 0);
            // create the color texture using the object provided in hpp file
            // with formate rgba and size of windowsize
            // then attach the texture to the frame buffer binded to GL_DRAW_FRAMEBUFFER with level 0

            depthTarget = texture_utils::empty(GL_DEPTH_COMPONENT24, windowSize);
            glFramebufferTexture2D(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depthTarget->getOpenGLName(), 0);
            // create the depth texture
            // with formate (Depth component with 24 bits) and size of windowsize
            // then attach the texture to the same frame buffer binded to GL_DRAW_FRAMEBUFFER with level 0

            if (glCheckFramebufferStatus(GL_DRAW_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
            {

                std::cout << "************************ Frame Buffer incomplete ****************************" << std::endl;
            }

            // TODO: (Req 11) Unbind the framebuffer just to be safe
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);

            // Create a vertex array to use for drawing the texture
            glGenVertexArrays(1, &postProcessVertexArray);

            // Create a sampler to use for sampling the scene texture in the post processing shader
            Sampler *postprocessSampler = new Sampler();
            postprocessSampler->set(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            postprocessSampler->set(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            postprocessSampler->set(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            postprocessSampler->set(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            // Create the post processing shader
            ShaderProgram *postprocessShader = new ShaderProgram();
            postprocessShader->attach("assets/shaders/fullscreen.vert", GL_VERTEX_SHADER);
            postprocessShader->attach(config.value<std::string>("postprocess", ""), GL_FRAGMENT_SHADER);
            //Setting Unifrom Values
            resolution = config.value("resolution", resolution);
            radius = config.value("radius", radius);
            dir_x = config.value("dir_x", dir_x);
            dir_y = config.value("dir_y", dir_y);
            dir={dir_x,dir_y};
            postprocessShader->link();



            // Create a post processing material
            postprocessMaterial = new TexturedMaterial();
            postprocessMaterial->shader = postprocessShader;
            postprocessMaterial->texture = colorTarget;
            postprocessMaterial->sampler = postprocessSampler;
            // The default options are fine but we don't need to interact with the depth buffer
            // so it is more performant to disable the depth mask
            postprocessMaterial->pipelineState.depthMask = false;
        }
    }

    void ForwardRenderer::destroy()
    {
        // Delete all objects related to the sky
        if (skyMaterial)
        {
            delete skySphere;
            delete skyMaterial->shader;
            delete skyMaterial->texture;
            delete skyMaterial->sampler;
            delete skyMaterial;
        }
        // Delete all objects related to post processing
        if (postprocessMaterial)
        {
            glDeleteFramebuffers(1, &postprocessFrameBuffer);
            glDeleteVertexArrays(1, &postProcessVertexArray);
            delete colorTarget;
            delete depthTarget;
            delete postprocessMaterial->sampler;
            delete postprocessMaterial->shader;
            delete postprocessMaterial;
        }
    }

    void ForwardRenderer::render(World *world)
    {
        // First of all, we search for a camera and for all the mesh renderers
        CameraComponent *camera = nullptr;
        opaqueCommands.clear();
        transparentCommands.clear();
        Lights.clear();

        for (auto entity : world->getEntities())
        {
            // If we hadn't found a camera yet, we look for a camera in this entity
            if (!camera)
                camera = entity->getComponent<CameraComponent>();
            // If this entity has a mesh renderer component
            if (auto meshRenderer = entity->getComponent<MeshRendererComponent>(); meshRenderer)
            {
                RenderCommand command;
                command.localToWorld = meshRenderer->getOwner()->getLocalToWorldMatrix();
                command.center = glm::vec3(command.localToWorld * glm::vec4(0, 0, 0, 1));

                command.mesh = meshRenderer->mesh;
                command.material = meshRenderer->material;
                // if it is transparent, we add it to the transparent commands list
                if (command.material->transparent)
                {
                    transparentCommands.push_back(command);
                }
                else
                {
                    // Otherwise, we add it to the opaque command list
                    opaqueCommands.push_back(command);
                }
            }
            // If this entity has a light component add it for the light vectore
            if (auto lightComp = entity->getComponent<LightingComponent>(); lightComp)
            {
                Lights.push_back(lightComp);
            }
        }
        // cout << "size for entities list" << world->getEntities().size() << "Size of Marked for removal" << world->getmarkedForRemoval().size() << endl;

        // If there is no camera, we return (we cannot render without a camera)
        if (camera == nullptr)
            return;

        // TODO: (Req 9) Modify the following line such that "cameraForward" contains a vector pointing the camera forward direction
        //  HINT: See how you wrote the CameraComponent::getViewMatrix, it should help you solve this one
        auto M = camera->getOwner()->getLocalToWorldMatrix();             // get local matrix
        glm::vec3 eyeTrans = M * glm::vec4(0, 0, 0, 1.0);                 // eye direction
        glm::vec3 centerTrans = M * glm::vec4(0, 0, -1, 1.0);             // center direction
        glm::vec3 cameraForward = glm::normalize(centerTrans - eyeTrans); // get camera forward direction
        std::sort(transparentCommands.begin(), transparentCommands.end(), [cameraForward](const RenderCommand &first, const RenderCommand &second)
                  {
                      // TODO: (Req 9) Finish this function
                      //  HINT: the following return should return true "first" should be drawn before "second".
                      return (glm::dot(cameraForward, first.center) > glm::dot(cameraForward, second.center)); // sort render components
                  });

        // TODO: (Req 9) Get the camera ViewProjection matrix and store it in VP
        //  multiply View Matrix and Projection Matrix
        glm::mat4 VP = camera->getProjectionMatrix(windowSize) * camera->getViewMatrix();

        // TODO: (Req 9) Set the OpenGL viewport using viewportStart and viewportSize

        // Parameters (glViewport):
        // x, y
        // Specify the lower left corner of the viewport rectangle, in pixels. The initial value is (0,0).
        // width, height
        // Specify the width and height of the viewport. When a GL context is first attached to a window, width and height are set to the dimensions of that window.

        glViewport(0, 0, windowSize[0], windowSize[1]);

        // TODO: (Req 9) Set the clear color to black and the clear depth to 1
        glEnable(GL_DEPTH_TEST); // do depth comparisons and update the depth buffer
        glDepthFunc(GL_LESS);    // Passes if the incoming depth value is less than the stored depth value.
        glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
        glClearDepth(1.0);
        // TODO: (Req 9) Set the color mask to true and the depth mask to true (to ensure the glClear will affect the framebuffer)
        glColorMask(true, true, true, true); // enable  writing of frame buffer color components
        glDepthMask(true);                   // enable writing into the depth buffer
        // If there is a postprocess material, bind the framebuffer
        if (postprocessMaterial && applyPostProcessing)
        {
            // TODO: (Req 11) bind the framebuffer
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, postprocessFrameBuffer);
        }

        // TODO: (Req 9) Clear the color and depth buffers
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
        // TODO: (Req 9) Draw all the opaque commands
        //  Don't forget to set the "transform" uniform to be equal the model-view-projection matrix for each render command
        for (auto it : opaqueCommands)
        {
            it.material->setup(); // set up material

            // if it is a light material add the camera_position,VP,M,M_IT,light_count,sky to shader
            if (auto light_material = dynamic_cast<LightingMaterial *>(it.material); light_material)
            {
                light_material->shader->set("VP", VP);

                light_material->shader->set("camera_position", eyeTrans);

                light_material->shader->set("M", it.localToWorld);

                light_material->shader->set("M_IT", glm::transpose(glm::inverse(it.localToWorld)));

                light_material->shader->set("light_count", (int)Lights.size());

                light_material->shader->set("sky.top", glm::vec3(0.5, 0.5, 0.5));
                light_material->shader->set("sky.horizon", glm::vec3(0.5, 0.5, 0.5));
                light_material->shader->set("sky.bottom", glm::vec3(0.5, 0.5, 0.5));

                // loop on all light componnet and add them to shader
                for (int i = 0; i < Lights.size(); i++)
                {

                    glm::vec3 light_position = Lights[i]->getOwner()->getLocalToWorldMatrix() * glm::vec4(Lights[i]->position, 1);
                    glm::vec3 light_direction = Lights[i]->getOwner()->getLocalToWorldMatrix() * glm::vec4(Lights[i]->direction, 0);

                    light_material->shader->set("lights[" + std::to_string(i) + "].type", (int)Lights[i]->kind);
                    light_material->shader->set("lights[" + std::to_string(i) + "].diffuse", Lights[i]->diffuse);
                    light_material->shader->set("lights[" + std::to_string(i) + "].specular", Lights[i]->specular);
                    // light_material->shader->set("lights["+std::to_string(i)+"].attenuation", Lights[i]->attenuation);
                    switch (Lights[i]->kind)
                    {
                    case 0: // case directional light
                        light_material->shader->set("lights[" + std::to_string(i) + "].direction", light_direction);
                        break;
                    case 2: // case spot light
                        light_material->shader->set("lights[" + std::to_string(i) + "].position", light_position);
                        light_material->shader->set("lights[" + std::to_string(i) + "].direction", light_direction);
                        light_material->shader->set("lights[" + std::to_string(i) + "].cone_angles", Lights[i]->cone_angles);
                        light_material->shader->set("lights[" + std::to_string(i) + "].attenuation", Lights[i]->attenuation);
                        break;
                    case 1: // case point light
                        light_material->shader->set("lights[" + std::to_string(i) + "].position", light_position);
                        light_material->shader->set("lights[" + std::to_string(i) + "].attenuation", Lights[i]->attenuation);
                        break;
                    }
                }
            }
            else
            {
                it.material->shader->set("transform", VP * it.localToWorld); // sent transform matrix to shader
            }

            it.mesh->draw(); // draw
        }
        // If there is a sky material, draw the sky
        if (this->skyMaterial)
        {
            // TODO: (Req 10) setup the sky material
            skyMaterial->setup();
            // TODO: (Req 10) Get the camera position
            glm::vec3 camera_position = camera->getOwner()->getLocalToWorldMatrix() * glm::vec4(0, 0, 0, 1);
            // cout << camera_position[0] <<" " <<camera_position[1]<<" " << camera_position[2] << endl;
            //  TODO: (Req 10) Create a model matrix for the sy such that it always follows the camera (sky sphere center = camera position)
            glm::mat4 trans = glm::mat4(1.0f);
            trans = glm::translate(trans, camera_position);
            // TODO: (Req 10) We want the sky to be drawn behind everything (in NDC space, z=1)
            //  We can acheive the is by multiplying by an extra matrix after the projection but what values should we put in it?
            //  scaled the Z with zero then translate it with 1 so it is in the far Z direction so any thing will be drawn above it
            glm::mat4 alwaysBehindTransform = glm::mat4(
                1.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 1.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 0.0f, 0.0f,
                0.0f, 0.0f, 1.0f, 1.0f);
            // TODO: (Req 10) set the "transform" uniform
            glm::mat4 Transformation = alwaysBehindTransform * VP * trans;
            skyMaterial->shader->set("transform", Transformation);

            // TODO: (Req 10) draw the sky sphere
            skySphere->draw();
        }
        // TODO: (Req 9) Draw all the transparent commands
        //  Don't forget to set the "transform" uniform to be equal the model-view-projection matrix for each render command
        for (auto it : transparentCommands)
        {
            it.material->setup(); // set up material

            // if it is a light material add the camera_position,VP,M,M_IT,light_count,sky to shader
            if (auto light_material = dynamic_cast<LightingMaterial *>(it.material); light_material)
            {
                light_material->shader->set("VP", VP);

                light_material->shader->set("camera_position", eyeTrans);

                light_material->shader->set("M", it.localToWorld);

                light_material->shader->set("M_IT", glm::transpose(glm::inverse(it.localToWorld)));

                light_material->shader->set("light_count", (int)Lights.size());

                light_material->shader->set("sky.top", glm::vec3(0.5, 0.5, 0.5));
                light_material->shader->set("sky.horizon", glm::vec3(0.5, 0.5, 0.5));
                light_material->shader->set("sky.bottom", glm::vec3(0.5, 0.5, 0.5));

                // loop on all light componnet and add them to shader
                for (int i = 0; i < Lights.size(); i++)
                {

                    glm::vec3 light_position = Lights[i]->getOwner()->getLocalToWorldMatrix() * glm::vec4(Lights[i]->position, 1);
                    glm::vec3 light_direction = Lights[i]->getOwner()->getLocalToWorldMatrix() * glm::vec4(Lights[i]->direction, 0);

                    light_material->shader->set("lights[" + std::to_string(i) + "].type", (int)Lights[i]->kind);
                    light_material->shader->set("lights[" + std::to_string(i) + "].diffuse", Lights[i]->diffuse);
                    light_material->shader->set("lights[" + std::to_string(i) + "].specular", Lights[i]->specular);
                    // light_material->shader->set("lights["+std::to_string(i)+"].attenuation", Lights[i]->attenuation);
                    switch (Lights[i]->kind)
                    {
                    case 0: // case directional light
                        light_material->shader->set("lights[" + std::to_string(i) + "].direction", light_direction);
                        break;
                    case 2: // case spot light
                        light_material->shader->set("lights[" + std::to_string(i) + "].position", light_position);
                        light_material->shader->set("lights[" + std::to_string(i) + "].direction", light_direction);
                        light_material->shader->set("lights[" + std::to_string(i) + "].cone_angles", Lights[i]->cone_angles);
                        light_material->shader->set("lights[" + std::to_string(i) + "].attenuation", Lights[i]->attenuation);
                        break;
                    case 1: // case point light
                        light_material->shader->set("lights[" + std::to_string(i) + "].position", light_position);
                        light_material->shader->set("lights[" + std::to_string(i) + "].attenuation", Lights[i]->attenuation);
                        break;
                    }
                }
            }
            else
            {
                it.material->shader->set("transform", VP * it.localToWorld); // sent transform matrix to shader
            }

            it.mesh->draw(); // draw
        }

        // If there is a postprocess material, apply postprocessing
        if (postprocessMaterial && applyPostProcessing)
        {
            // TODO: (Req 11) Return to the default framebuffer
            glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
            // unbind the frameBuffer to return to the default frameBuffer

            // TODO: (Req 11) Setup the postprocess material and draw the fullscreen triangle
            postprocessMaterial->setup(); // set up material

            // drawing a full screen triangle
            glBindVertexArray(postProcessVertexArray);

            //Set Uniform Values
            postprocessMaterial->shader->set("resolution",resolution);
            postprocessMaterial->shader->set("radius",radius);
            postprocessMaterial->shader->set("dir",dir);
            glDrawArrays(GL_TRIANGLES, 0, 3);
        }
    }
    void ForwardRenderer::setApplyPostProcessing(bool value)
    {
        applyPostProcessing = value;
    }
}