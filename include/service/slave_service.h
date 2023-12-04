#ifndef __SLAVE_SERVICE_H__
#define __SLAVE_SERVICE_H__
#include <memory>
#include <mutex>
#include <unordered_map>

#include "model/ScoreMatrixBlock.h"
#include "model/ScoreMatrixTask.hpp"
#include "model/ScoreMatrixTaskResponse.hpp"
#include "model/TracebackTask.hpp"
#include "model/TracebackTaskResponse.hpp"
#include "service/abstract_service.h"
class SlaveService : public AbstractService {
   public:
    SlaveService(AbstractController* controller)
        : AbstractService(controller) {}
    virtual void onInit() override;
    virtual void onNewMessage(std::string peer_id, const std::string& message,
                              bool is_binary) override;
    virtual void onConnectionEstablished(const std::string peer_id) override;
    virtual void onConnectionTerminated(const std::string peer_id) override;

   private:
    // x_y -> ScoreMatrixBlock
    std::mutex lock_;
    std::unordered_map<std::string, std::shared_ptr<ScoreMatrixBlock>>
        score_matrix_blocks_;

   private:
    std::shared_ptr<ScoreMatrixTaskResponse> onScoreMatrixTask(
        std::shared_ptr<ScoreMatrixTask> task);
    std::shared_ptr<TracebackTaskResponse> onTracebackTask(
        std::shared_ptr<TracebackTask> task);
    void sendResultBack(std::shared_ptr<AbstractJsonObject> obj);

    std::shared_ptr<ScoreMatrixBlock> calculateScoreMatrixBlock(
        std::shared_ptr<ScoreMatrixTask> task);

    std::shared_ptr<TracebackTaskResponse> traceBackOnBlock(
        std::shared_ptr<TracebackTask> task,
        std::shared_ptr<ScoreMatrixBlock> block);

    int getScore(
        int x, int y,
        const std::vector<std::vector<std::pair<int, int>>>& score_matrix,
        const std::vector<int> left_column, const std::vector<int> top_row,
        int left_top_number);
    int getScore(int x, int y, std::shared_ptr<ScoreMatrixBlock> res);
};

#endif