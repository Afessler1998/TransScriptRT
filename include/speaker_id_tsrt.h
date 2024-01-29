#ifndef speaker_id_tsrt_h
#define speaker_id_tsrt_h

#include <memory>
#include <string>

/**
 * @brief Represents a speaker.
 * 
 * Speaker_ID is a wrapper for a float array of speaker embeddings. It also contains
 * the name of the speaker and the size of the float array.
 * 
 * @param name The name of the speaker.
 * @param vector A float array of speaker embeddings.
 * @param vector_size The size of the float array.
*/
class Speaker_ID {

public:

    std::string name;
    std::unique_ptr<float[]> vector;
    size_t vector_size;

    // Default constructor
    Speaker_ID(std::string n, std::unique_ptr<float[]> v, size_t vs) 
        : name(std::move(n)), vector(std::move(v)), vector_size(vs) {}

    // Copy constructor
    Speaker_ID(const Speaker_ID &other) {
        this->name = other.name;
        this->vector = std::make_unique<float[]>(other.vector_size);
        memcpy(this->vector.get(), other.vector.get(), other.vector_size * sizeof(float));
    }

    // Move constructor
    Speaker_ID(Speaker_ID &&other) noexcept {
        this->name = std::move(other.name);
        this->vector = std::move(other.vector);
        this->vector_size = other.vector_size;
    }

    // Copy assignment operator
    Speaker_ID& operator=(const Speaker_ID &other) {
        this->name = other.name;
        this->vector = std::make_unique<float[]>(other.vector_size);
        memcpy(this->vector.get(), other.vector.get(), other.vector_size * sizeof(float));
        return *this;
    }

    // Move assignment operator
    Speaker_ID& operator=(Speaker_ID &&other) noexcept {
        this->name = std::move(other.name);
        this->vector = std::move(other.vector);
        this->vector_size = other.vector_size;
        return *this;
    }

    // TODO: IMPLEMENT A UNIQUE IDENTIFIER FOR EACH SPEAKER AND USE THAT FOR THE EQUALITY OPERATOR 
    // Equality operator
    bool operator==(const std::string &name) const {
        return this->name == name;
    }
};

#endif
