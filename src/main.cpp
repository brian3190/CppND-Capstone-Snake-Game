#include <iostream>
#include <future>
#include <mutex>
#include <algorithm>
#include <thread>
#include <vector>
#include "controller.h"
#include "game.h"
#include "renderer.h"

class CheerMessage
{
  public:
    CheerMessage(int id) : _id(id) {}
    int getID() { return _id; }

  private:
    int _id;
};

class Cheer
{
  public:
    Cheer() {}
    CheerMessage popBack()
    {
      // perform vector modification under the lock
      std::unique_lock<std::mutex> uLock(_mutex);
      _cond.wait(uLock, [this] { return !_cheers.empty(); }); // pass unique lock to condition variable

      // remove last vector element from queue
      CheerMessage chr = std::move(_cheers.back());
      _cheers.pop_back();

      return chr;
    }
    
    void pushBack(CheerMessage &&chr)
    {
      // simulate some work
      std::this_thread::sleep_for(std::chrono::milliseconds(100));

      // perform vector modification under the lock
      std::lock_guard<std::mutex> uLock(_mutex);

      std::cout << "Cheer #" << chr.getID() << ": Keep on playing!" << std::endl;
      _cheers.push_back(std::move(chr));
      _cond.notify_one();
    }

  private:
    std::mutex _mutex;
    std::condition_variable _cond;
    std::vector<CheerMessage> _cheers;
};

int main() {
  constexpr std::size_t kFramesPerSecond{60};
  constexpr std::size_t kMsPerFrame{1000 / kFramesPerSecond};
  constexpr std::size_t kScreenWidth{1280};
  constexpr std::size_t kScreenHeight{800};
  constexpr std::size_t kGridWidth{50};
  constexpr std::size_t kGridHeight{50};

  // create monitor object as a shared pointer 
  std::shared_ptr<Cheer> queue(new Cheer);

  Renderer renderer(kScreenWidth, kScreenHeight, kGridWidth, kGridHeight);
  Controller controller;
  Game game(kGridWidth, kGridHeight);
  std::cout << "Spawning threads to motivate you..." << std::endl;
  std::vector<std::future<void>> futures;
  for(int i = 0; i < 3; ++i)
  {
    CheerMessage m(i);
    futures.emplace_back(std::async(std::launch::async, &Cheer::pushBack, queue, std::move(m)));
  }
  std::cout << "Cheering you up as you play..." << std::endl;
  //monitor object pattern
  while(true)
  {
    CheerMessage chr = queue->popBack();
    std::cout << "    Cheer #" << chr.getID() << ": Don't bite your tail! " << std::endl;
  }

  std::for_each(futures.begin(), futures.end(), [](std::future<void> &ftr){
    ftr.wait();
  });

  game.Run(controller, renderer, kMsPerFrame);
  std::cout << "Game has terminated successfully!\n";
  std::cout << "Score: " << game.GetScore() << "\n";
  std::cout << "Size: " << game.GetSize() << "\n";
  return 0;
}