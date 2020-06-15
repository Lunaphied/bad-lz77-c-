#include <iostream>
#include <string>
#include <vector>
#include <cstdint>
#include <utility>

const std::string myInputStr = "Clojure provides array functions through aget and aset (slightly strange in that it mutates a data structure in place). Using Java arrays isn't very functional, but in my opinion this is about providing the balance between strictness (e.g. Haskell) and leniency (e.g. C)AAAAAAAAAAAAAAAAAAAAAAAAAleniencyBleniencyAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAAA";
// probably in real life use a log2 number here, we use 20 to simulate a small buffer for a short string
constexpr auto HISTORY_LEN = 20;

// I had trouble coming up with good typenames here sorry
struct backref {
  size_t length;
  size_t distance;
};

struct seq_element {
  bool is_literal;
  union {
    char literal;
    backref match;
  };
};

// much simpler c style pointer based version without too much safety
// TODO: can you do this using iterators even if you extend the size of the vector?
// no idea..
// also this whole mess doesn't work right if the pointer math doesn't hold
// I feel like other implementations I've seen have less of it
backref findBackrefC(uint8_t* part, size_t part_size, uint8_t* history, size_t hist_size) {
  backref best_match = {0, 0};
  // keep track of the end of history... is this how that's typically done?
  // now that I go to do it myself I honestly can't remember
  uint8_t* hist_end = history+hist_size;
  uint8_t* part_end = part+part_size;
  uint8_t* orig_history = history;
  // try all potential starting positions in history
  while (history != hist_end) {
    // account for 0 distance being actually before hist_end
    backref local_match = {0, static_cast<size_t>(hist_end-history-1)};
    std::string match;
    // use this pointer to actually iterate along part
    uint8_t* pp = part;
    // same but use this one to keep track of where we are along history
    uint8_t* hh = history;
    std::cout << "matching " << *pp << "==" << *hh << "\n";
    // oops we need to actually use part_end...
    while (pp != part_end) {
      // No match
      if (*pp != *hh) {
        break;
      }
      if (pp != part) {
        std::cout << "matching " << *pp << "==" << *hh << "\n";
      }
      match += *hh;
      local_match.length++;
      pp++;
      hh++;
      // wrap around?
      if (hh >= hist_end) {
        hh = history;
      }
    }
    if (best_match.length < local_match.length) {
      best_match = local_match;
      std::string a((char*)part, part_size);
      // have to cap the input buffer for rle
      uint8_t* matchStart = hist_end-1-best_match.distance;
      uint8_t* matchEnd = matchStart+best_match.length;
      if (matchEnd > hist_end) {
        matchEnd = hist_end;
      }
      std::string b(matchStart, matchEnd);
      std::cout << "was matching: " << a << '\n';
      std::cout << "in          : " << b << '\n';
      std::cout << "got match   : " << match << '\n';
      std::cout << "dist=" << best_match.distance << "\tlength=" <<
        best_match.length << '\n';
      std::cout << std::endl;
    }
    history++;
  }
  return best_match;
}


// It would be nice if a function like this actually was implemented in such a way to support
// state being captured as a sort of stack and allowed backtracking to try out shorter matches
// for a better match to follow, but that's fancy...

// oops I wrote this and really should have used iterators because c++ only added a subvector thing in c++20 and we could also probably make some sort of iterator contraption to handle
// the RLE infinite history buffer thing
backref findBackref(std::vector<uint8_t> part, std::vector<uint8_t> history) {
  // 0 is an invalid length but 0 is a valid distance (means start at last position of history)
  // code is kind of confusing because I started with i = history.size() - 1 instead of i = 0 distance, index=history.size() - 1 - i
  backref best_match = {0, 0};
  // not using auto because it might infer unsigned and break for loops? dunno
  for (ssize_t i = history.size()-1; i >= 0; i--) {
    // we test for matches starting from a backref expressed as a positive distance (0 is end of buffer)
    // convert i to this form
    auto backref_form = history.size() - i - 1;
    //std::cout << "backref_form: " << backref_form << '\n';
    backref this_match = {0, backref_form};
    // This section might be super messy because I forgot to
    // implement the key feature which is RLE when we can
    // run off the end of the buffer
    int part_idx = 0;

    // Keep looping until we run out of part to match
    while (part_idx < part.size()) {
      // Make sure it actually matches and increase match length (starting from 0 this will work for every position)
      int hist_idx = i+this_match.length;
      // we have to keep his_idx matching, if it runs off the
      // end we bring it back to the start of the part
      if (hist_idx >= history.size()) {
        // restart at the beginning of this match
        hist_idx = i;
        //std::cout << "wraparound\n";
      }

      //std::cout << "matching " << part[part_idx] <<"=="<<history[hist_idx] << '\n';

      if (part[part_idx] == history[hist_idx]) {
        this_match.length++;
      } else {
        // Otherwise this match is finished (no gaps)
        break;
      }
      // make sure to update this each loop... while loops make
      // me scared because of things like this...
      part_idx += 1;
    }

    // Skip anything else for no matches
    if (this_match.length <= 0) {
      continue;
    }

    // We only optimize for long matches (technically you want to optimize for bit usage so shorter distances are also good)
    if (this_match.length > best_match.length) {
      best_match = this_match;
    }
  }
  return best_match;
}
// Just output length distance pairs
int main() {
  std::vector<uint8_t> input_buffer(myInputStr.begin(), myInputStr.end());
  std::cout << input_buffer.data() << '\n';
  std::vector<seq_element> output;
  for (auto i = 0; i < input_buffer.size(); i++) {
    // oops
    std::vector<uint8_t> history;
    // Do this to make it properly be a last HISTORY_LEN buffer
    // not sure in c++ how best to do this since technically I
    // think we end up copy constructing these after constructing
    // an empty vector
    if (i >= HISTORY_LEN) {
      history = std::vector<uint8_t>(input_buffer.begin()+i-HISTORY_LEN,
       input_buffer.begin()+i);
    } else {
      history = std::vector<uint8_t>(input_buffer.begin(),
        input_buffer.begin()+i);
    }
    std::vector<uint8_t> part(input_buffer.begin()+i, input_buffer.end());

    
    std::cout << "history: " << std::string(history.begin(), history.end()) << '\n';
    std::cout << "part: " << std::string(part.begin(), part.end()) << '\n';
    

    auto match = findBackrefC(part.data(), part.size(), history.data(), history.size());
    //auto match = findBackref(part, history);

    /*
    std::cout << "match len=" << match.length << '\n' << "dist=" << match.distance << '\n';
    std::cout << "match str=" << std::string(history.end()-match.distance-1, history.end()-match.distance-1+match.length) << '\n';
    */

    seq_element out_elem;
    // in reality we'd want to make sure the match was long enough to justify not just using a literal and maybe non-greadily consume matches
    if (match.length > 0) {
      // if match output the match
      out_elem.is_literal = false;
      out_elem.match = match;
      // oops we need to advance past the match now
      i += match.length-1;
    } else {
      // otherwise literal
      out_elem.is_literal = true;
      out_elem.literal = part[0];
    }
    output.push_back(out_elem);
  }
  
  for (const auto& i : output) {
    // Print out our compressed form
    if (i.is_literal) {
      std::cout << "literal val=" << i.literal << '\n';
    } else {
      std::cout << "backref dist=" << i.match.distance << " length=" << i.match.length << '\n';
    }
  } 
  

  // try to reconstruct
  std::vector<char> reconstruct;
  // c++ makes us do this after initial construction because
  // declaring the size first makes it change it's size not just capacity
  // no way to create and reserve capacity without messing with size...
  reconstruct.reserve(input_buffer.size());

  for (const auto& i : output) {
    if (i.is_literal) {
      reconstruct.push_back(i.literal);
    } else {
      // this is bad anyway we should really use some sort
      // of circular history buffer (in c we just use pointers
      // into the input/output streams) but I don't like that
      // because it gets messy/doesn't work for doing decompress
      // in chunks
      // just use a byte by byte copy (rip easy copy optimizations)
      int start_position = reconstruct.size() - i.match.distance - 1;
      int last_position = start_position + i.match.length;
      if (last_position > reconstruct.size()) {
        /*
        std::cout << "start=" << start_position << '\n'
                  << "last="  << last_position  << '\n'
                  << "hist_size=" << reconstruct.size() << '\n';
        std::cout << "wtf match.distance=" << i.match.distance << '\n'
                  << "match.length=" << i.match.length << '\n';
        */
        // byte by byte
        for (auto p = start_position; p < last_position; p++) {
          reconstruct.push_back(reconstruct[p]);
        }
        //std::cout << "chunk = " << std::string(reconstruct.begin()+start_position, reconstruct.begin()+last_position) << '\n';
      } else {
        // Don't need to do anything to handle cycling
        //std::cout << "backref should be = " << std::string(input_buffer.begin()+reconstruct.size()-i.match.distance-1, input_buffer.begin()+reconstruct.size()-i.match.distance-1 + i.match.length) << '\n';
        reconstruct.insert(reconstruct.end(),
          reconstruct.end()-1-i.match.distance,
          reconstruct.end()-1-i.match.distance+i.match.length);
      }
      //reconstruct.insert(
    }
  }
  std::cout << reconstruct.size() << '\n';
  std::string outstring = std::string(reconstruct.begin(), reconstruct.end());
  std::cout << "output: " << outstring << '\n'; 
  std::cout << "same string as input? " << (outstring == myInputStr) << '\n';
  
  // for debugging
  if (outstring != myInputStr) {
    for (auto off = 0; off < outstring.size(); off++) {
      auto part1 = outstring.substr(0, off);
      auto foundPos = myInputStr.find(part1);
      std::cout << off << ": " << (foundPos == std::string::npos) << '\n';
      if (foundPos == std::string::npos) {
        std::cout << "bit     : " << outstring.substr(off) << '\n';
        std::cout << "real bit: " << myInputStr.substr(off) << '\n';
      }
    }
  }
  return 0;
}