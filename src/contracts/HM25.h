using namespace QPI;

struct HM252
{
};

// Variables d'état globales (en dehors de la classe)
static uint64 totalLoans;
static uint64 totalRepaid;
static uint64 totalVolume;
static uint64 activeLoans;
static uint32 loanCount;
static uint32 repCount;

struct Loan
{
    uint64 amount;
    uint64 timestamp;
    uint64 dueDate;
    bool repaid;
    id borrowerId;
};

struct Reputation
{
    uint64 score;
    uint64 loansRepaid;
    uint64 totalBorrowed;
    uint64 totalRepaid;
};

static Loan loans[100];
static Reputation reputations[100];
static id loanKeys[100];
static id repKeys[100];

struct HM25 : public ContractBase
{
public:
   struct MakeLoan_input
   {
       uint64 amount;
   };
   struct MakeLoan_output
   {
   };

   struct RepayLoan_input
   {
       uint64 amount;
   };
   struct RepayLoan_output
   {
   };

   struct GetLoan_input
   {
       id borrowerId;
   };
   struct GetLoan_output
   {
       uint64 amount;
       uint64 timestamp;
       uint64 dueDate;
       bool repaid;
       bool exists;
   };

   struct GetStats_input
   {
   };
   struct GetStats_output
   {
       uint64 totalLoans;
       uint64 totalRepaid;
       uint64 totalVolume;
       uint64 activeLoans;
   };

   struct GetReputation_input
   {
       id userId;
   };
   struct GetReputation_output
   {
       uint64 score;
       uint64 loansRepaid;
       uint64 totalBorrowed;
       uint64 totalRepaid;
   };

   PUBLIC_PROCEDURE(MakeLoan)
       uint64 amountInQU = input.amount * 1000000000ULL;
       
       if (amountInQU < 10000000000ULL || amountInQU > 1000000000000ULL)
       {
           return;
       }

       // Find existing loan
       sint32 existingLoanIndex = -1;
       for (uint32 i = 0; i < loanCount; i++)
       {
           if (loanKeys[i] == qpi.invocator())
           {
               existingLoanIndex = i;
               break;
           }
       }

       if (existingLoanIndex >= 0 && !loans[existingLoanIndex].repaid)
       {
           return;
       }

       if (loanCount >= 100)
       {
           return;
       }

       loans[loanCount].amount = amountInQU;
       loans[loanCount].timestamp = qpi.tick();
       loans[loanCount].dueDate = qpi.tick() + (30 * 24 * 60);
       loans[loanCount].repaid = false;
       loans[loanCount].borrowerId = qpi.invocator();
       loanKeys[loanCount] = qpi.invocator();
       loanCount++;

       totalLoans++;
       totalVolume += amountInQU;
       activeLoans++;

       qpi.transfer(qpi.invocator(), amountInQU);
   _

   PUBLIC_PROCEDURE(RepayLoan)
       uint64 repayAmountInQU = input.amount * 1000000000ULL;
       
       // Find loan
       sint32 loanIndex = -1;
       for (uint32 i = 0; i < loanCount; i++)
       {
           if (loanKeys[i] == qpi.invocator())
           {
               loanIndex = i;
               break;
           }
       }

       if (loanIndex < 0 || loans[loanIndex].repaid)
       {
           return;
       }

       if (repayAmountInQU != loans[loanIndex].amount)
       {
           return;
       }

       if (qpi.invocationReward() < repayAmountInQU)
       {
           return;
       }

       loans[loanIndex].repaid = true;
       totalRepaid += repayAmountInQU;
       activeLoans--;

       // Find reputation
       sint32 repIndex = -1;
       for (uint32 i = 0; i < repCount; i++)
       {
           if (repKeys[i] == qpi.invocator())
           {
               repIndex = i;
               break;
           }
       }

       if (repIndex < 0 && repCount < 100)
       {
           repIndex = repCount;
           repKeys[repCount] = qpi.invocator();
           reputations[repCount].score = 0;
           reputations[repCount].loansRepaid = 0;
           reputations[repCount].totalBorrowed = 0;
           reputations[repCount].totalRepaid = 0;
           repCount++;
       }

       if (repIndex >= 0)
       {
           reputations[repIndex].score++;
           reputations[repIndex].loansRepaid++;
           reputations[repIndex].totalRepaid += repayAmountInQU;
           
           if (qpi.tick() <= loans[loanIndex].dueDate)
           {
               reputations[repIndex].score++;
           }
       }

       qpi.burn(repayAmountInQU);
   _

   PUBLIC_FUNCTION(GetLoan)
       // Find loan
       sint32 loanIndex = -1;
       for (uint32 i = 0; i < loanCount; i++)
       {
           if (loanKeys[i] == input.borrowerId)
           {
               loanIndex = i;
               break;
           }
       }

       if (loanIndex >= 0)
       {
           output.amount = loans[loanIndex].amount;
           output.timestamp = loans[loanIndex].timestamp;
           output.dueDate = loans[loanIndex].dueDate;
           output.repaid = loans[loanIndex].repaid;
           output.exists = true;
       }
       else
       {
           output.amount = 0;
           output.timestamp = 0;
           output.dueDate = 0;
           output.repaid = false;
           output.exists = false;
       }
   _

   PUBLIC_FUNCTION(GetStats)
       output.totalLoans = totalLoans;
       output.totalRepaid = totalRepaid;
       output.totalVolume = totalVolume;
       output.activeLoans = activeLoans;
   _

   PUBLIC_FUNCTION(GetReputation)
       // Find reputation
       sint32 repIndex = -1;
       for (uint32 i = 0; i < repCount; i++)
       {
           if (repKeys[i] == input.userId)
           {
               repIndex = i;
               break;
           }
       }

       if (repIndex >= 0)
       {
           output.score = reputations[repIndex].score;
           output.loansRepaid = reputations[repIndex].loansRepaid;
           output.totalBorrowed = reputations[repIndex].totalBorrowed;
           output.totalRepaid = reputations[repIndex].totalRepaid;
       }
       else
       {
           output.score = 0;
           output.loansRepaid = 0;
           output.totalBorrowed = 0;
           output.totalRepaid = 0;
       }
   _

   REGISTER_USER_FUNCTIONS_AND_PROCEDURES
       REGISTER_USER_PROCEDURE(MakeLoan, 1);
       REGISTER_USER_PROCEDURE(RepayLoan, 2);
       REGISTER_USER_FUNCTION(GetLoan, 1);
       REGISTER_USER_FUNCTION(GetStats, 2);
       REGISTER_USER_FUNCTION(GetReputation, 3);
   _

   INITIALIZE
       totalLoans = 0;
       totalRepaid = 0;
       totalVolume = 0;
       activeLoans = 0;
       loanCount = 0;
       repCount = 0;
   _
};