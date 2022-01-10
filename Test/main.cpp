#include "GLContext.h"
#include "VI.h"
#include "GL/glew.h"

int main(int argc, char* argv[]) {
    if (argc < 2) {
        return -1;
    }
    std::string type = argv[1];
    std::shared_ptr<VI::Camera> camera = nullptr;
    std::shared_ptr<VI::Video> video = nullptr;
    unsigned char* frame = NULL;
    int texWidth = 0;
    int texHeight = 0;
    if (type == "--camera") {
        auto devices = VI::Camera::getDeviceList();
        camera = std::make_shared<VI::Camera>(0, 1920, 1080, 0);
        camera->showSettingsWindow();
        texWidth = camera->getWidth();
        texHeight = camera->getHeight();
        frame = new unsigned char[texWidth * texHeight * 3];
    } else if (type == "--video") {
        if (argc < 3) {
            return -1;
        }
        std::string filePath = argv[2];
        video = std::make_shared<VI::Video>(VI::String(filePath.c_str(), filePath.size()));
        texWidth = video->getWidth();
        texHeight = video->getHeight();
        frame = new unsigned char[texWidth * texHeight * 3];
    } else {
        return -1;
    }

    GLFWContext::DeviceSettings deviceSettings;
#ifdef _WIN32
    deviceSettings.contextMajorVersion = 3;
    deviceSettings.contextMinorVersion = 3;
#elif __APPLE__
    deviceSettings.contextMajorVersion = 3;
    deviceSettings.contextMinorVersion = 2;
#endif

    GLFWContext::WindowSettings windowSettings;
    windowSettings.title = "Camera App";
    windowSettings.width = texWidth ? texWidth : 1920;
    windowSettings.height = texHeight ? texHeight : 1080;
    windowSettings.focused = false;
    windowSettings.floating = false;
    windowSettings.decorated = true;
    windowSettings.cursorMode = GLFWContext::Cursor::ECursorMode::NORMAL;
    windowSettings.cursorShape = GLFWContext::Cursor::ECursorShape::ARROW;
    auto glfwContext = std::make_shared<GLFWContext::Context>(windowSettings, deviceSettings);
    glfwContext->window->MakeCurrentContext(true);
    glfwContext->window->FramebufferResizeEvent += [](uint16_t width, uint16_t height) {
        // make sure the viewport matches the new window dimensions; note that width and
        // height will be significantly larger than specified on retina displays.
        glViewport(0, 0, width, height);
    };
    glfwContext->device->SetVsync(true);

    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float vertices[] = {
        // positions          // colors           // texture coords
        1.0f,  1.0f,  0.0f, 1.0f, 0.0f, 0.0f, 1.0f, 0.0f,  // top right
        1.0f,  -1.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, 1.0f,  // bottom right
        -1.0f, -1.0f, 0.0f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f,  // bottom left
        -1.0f, 1.0f,  0.0f, 1.0f, 1.0f, 0.0f, 0.0f, 0.0f   // top left
    };
    unsigned int indices[] = {
        0, 1, 3,  // first triangle
        1, 2, 3   // second triangle
    };
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);

    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    // position attribute
    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    // color attribute
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(3 * sizeof(float)));
    glEnableVertexAttribArray(1);
    // texture coord attribute
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 8 * sizeof(float), (void*)(6 * sizeof(float)));
    glEnableVertexAttribArray(2);

    unsigned int texture;
    glActiveTexture(GL_TEXTURE0);
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);  // all upcoming GL_TEXTURE_2D operations now have effect on this texture object
    // set the texture wrapping parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);  // set texture wrapping to GL_REPEAT (default wrapping method)
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
    // set texture filtering parameters
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, frame);
    glBindTexture(GL_TEXTURE_2D, 0);

    const std::string vs =
        "#version 330 core\n\
layout (location = 0) in vec3 aPos;\n\
layout (location = 1) in vec3 aColor;\n\
layout (location = 2) in vec2 aTexCoord;\n\
\n\
out vec3 ourColor;\n\
out vec2 TexCoord;\n\
\n\
void main()\n\
{\n\
	gl_Position = vec4(aPos, 1.0);\n\
	ourColor = aColor;\n\
	TexCoord = vec2(aTexCoord.x, aTexCoord.y);\n\
}";

    const std::string fs =
        "#version 330 core\n\
out vec4 FragColor;\n\
\n\
in vec3 ourColor;\n\
in vec2 TexCoord;\n\
\n\
// texture sampler\n\
uniform sampler2D texture1;\n\
\n\
void main()\n\
{\n\
	vec4 color = texture(texture1, TexCoord);\n\
	float grey = (color.x + color.y + color.z) / 3.0;\n\
	FragColor = vec4(grey, grey, grey, 1.0);\n\
	FragColor = color;\n\
	// FragColor = vec4(vec3(1.0 - color.x, 1.0 - color.y, 1.0 - color.z) * (0.7 + 0.3 * sin(TexCoord.x * 5 * 3.1415926)), 1.0);\n\
}";
    const char* vShaderCode = vs.c_str();
    const char* fShaderCode = fs.c_str();
    unsigned int vertex, fragment;
    // vertex shader
    vertex = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertex, 1, &vShaderCode, NULL);
    glCompileShader(vertex);
    // fragment Shader
    fragment = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragment, 1, &fShaderCode, NULL);
    glCompileShader(fragment);

    auto program = glCreateProgram();
    glAttachShader(program, vertex);
    glAttachShader(program, fragment);
    glLinkProgram(program);
    // delete the shaders as they're linked into our program now and no longer necessery
    glDeleteShader(vertex);
    glDeleteShader(fragment);

    const auto frameStartTime = glfwContext->device->GetElapsedTime();
    auto prevFrameTime = frameStartTime;
    int frames = 0;

    while (!glfwContext->window->ShouldClose()) {
        auto currentFrameTime = glfwContext->device->GetElapsedTime();
        if ((currentFrameTime - prevFrameTime) > 1.0 || frames == 0) {
            float fps = (float)(int)((double)frames / (currentFrameTime - prevFrameTime) * 10) / 10.f;
            glfwContext->window->SetTitle("Camera App (" + std::to_string(fps) + " FPS)");
            prevFrameTime = currentFrameTime;
            frames = 0;
        }
        frames++;

        glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        if (type == "--camera" && camera) {
            bool resized = false;
            auto curTexWidth = camera->getWidth();
            auto curTexHeight = camera->getHeight();
            if (texWidth != curTexWidth || texHeight != curTexHeight) {
                delete[] frame;
                texWidth = curTexWidth;
                texHeight = curTexHeight;
                frame = new unsigned char[texWidth * texHeight * 3];
                resized = true;
                glfwContext->window->SetSize(texWidth, texHeight);
            }
            if (camera->isFrameNew() || resized) {
                camera->getPixels(frame);
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texture);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                if (resized) {
                    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, texWidth, texHeight, 0, GL_RGB, GL_UNSIGNED_BYTE, frame);
                } else {
                    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texWidth, texHeight, GL_RGB, GL_UNSIGNED_BYTE, frame);
                }
                glBindTexture(GL_TEXTURE_2D, 0);
            }
        } else if (type == "--video" && video) {
            auto vTotalTime = video->getTotalTime();
            auto vTime = fmod((double)(currentFrameTime - frameStartTime), vTotalTime);
            if (video->retrieveFrame(vTime, &frame)) {
                glActiveTexture(GL_TEXTURE0);
                glBindTexture(GL_TEXTURE_2D, texture);
                glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
                glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, texWidth, texHeight, GL_RGB, GL_UNSIGNED_BYTE, frame);
                glBindTexture(GL_TEXTURE_2D, 0);
            }
        }
        // bind Texture
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, texture);

        // render container
        glUseProgram(program);
        auto location = glGetUniformLocation(program, "texture1");
        glUniform1i(location, 0);
        glBindVertexArray(VAO);
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
        glfwContext->window->SwapBuffers();
        glfwContext->device->PollEvents();
    }
    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);
    delete[] frame;
    return 0;
}
